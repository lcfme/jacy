/*
<package name="Camera_Android">
	<target n="android">
		<lib n="android/wrapper_java/camera.java"/>
		<array a="android_permissions" n="android.permission.CAMERA"/>
		<array a="android_features" n="android:hardware.camera"/>
	</target>
</package>
*/
#include "sdl.h"
#include <jni.h>
#include <string.h>
#include <android/log.h>
#include "wrapper_defines.h"
#define MAX_CAMERAS 8
typedef unsigned char u8;
typedef unsigned int u32;

typedef struct _TCamera{
	int m_inited;
	jobject m_cam_object;
	////////
	int m_is_on;
	SDL_mutex* m_cam_mutex;
	////////
	u32* m_image_front;
	u32* m_image_back;
	int m_w,m_h,m_image_ready;
	int m_android_texid;
	int m_android_is_texid_valid;
}TCamera;

static TCamera g_cameras[MAX_CAMERAS]={0};

//the output is BGR, (likely) in sRGB
//we can't have _ in the method name
//todo: write to a "backbuffer", the getter merely swaps them in a lock
JNIEXPORT void JNICALL Java_com_spap_wrapper_camera_sendresult(JNIEnv* env,jclass cls,jint cam_id,jbyteArray a,jint w,jint h){
	jboolean is_copy;
	TCamera* cam=&g_cameras[cam_id];
	u8* img_nv21;
	int need_post=0;
	//__android_log_print(ANDROID_LOG_ERROR,"STDOUT","image coming");
	SDL_LockMutex(cam->m_cam_mutex);
	if(!cam->m_is_on){
		SDL_UnlockMutex(cam->m_cam_mutex);
		return;
	}
	if(cam->m_w!=w||cam->m_h!=h){
		if(cam->m_image_back){
			free(cam->m_image_back);
			cam->m_image_back=NULL;
		}
		if(cam->m_image_front){
			free(cam->m_image_front);
			cam->m_image_front=NULL;
		}
		cam->m_w=w;
		cam->m_h=h;
	}
	if(!cam->m_image_back){
		cam->m_image_back=malloc(4*w*h);
	}
	img_nv21=(*env)->GetByteArrayElements(env,a,&is_copy);
	//convert NV21 to rgb
	{
		int size=w*h;
		int offset=size;
		u32* out=cam->m_image_back;
		int i,j,k;//,w3=w*3;
		for(i=0, k=0, j=0; i < size; ) {
			int y1=(int)img_nv21[i];
			int y2=(int)img_nv21[i+1];
			int y3=(int)img_nv21[w+i];
			int y4=(int)img_nv21[w+i+1];
			int v=(int)img_nv21[size+k]-128;
			int u=(int)img_nv21[size+k+1]-128;
			//int i3=i*3;
			u32* pt;
			int tmp;
			unsigned int tmp2;
			u32 val;
			//pt=out+i3;
			pt=out+i;
			//coulddo: neon?
			#define convertYUVtoARGB(y,u,v) val=0xff000000u;\
				tmp=y+((116130*v)>>16);			val+=(u32)(!(tmp2=tmp>>8)?tmp:(0xff^(tmp2>>24)));\
				tmp=y-((22544*v+46793*u)>>16);	val+=(u32)(!(tmp2=tmp>>8)?tmp:(0xff^(tmp2>>24)))<<8;\
				tmp=y+((91881*u)>>16);			val+=(u32)(!(tmp2=tmp>>8)?tmp:(0xff^(tmp2>>24)))<<16;\
				pt[0]=val;
			convertYUVtoARGB(y1, u, v);pt+=3;
			convertYUVtoARGB(y2, u, v);
			//pt=out+(w3+i3);
			pt=out+(w+i);
			convertYUVtoARGB(y3, u, v);pt+=3;
			convertYUVtoARGB(y4, u, v);
			#undef convertYUVtoARGB
			i+=2; k+=2; j+=2;
			if(j>=w){
				i+=j;
				j=0;
			}
		}
	}
	(*env)->ReleaseByteArrayElements(env,a,img_nv21,JNI_ABORT);
	cam->m_w=w;
	cam->m_h=h;
	cam->m_image_ready=1;
	SDL_UnlockMutex(cam->m_cam_mutex);
	{
		//ignore camera id, just send something
		SDL_Event a;
		memset(&a,0,sizeof(a));
		a.type=SDL_USEREVENT;
		a.user.code=3;
		SDL_PushEvent(&a);
	}
}

#ifdef PM_IS_LIBRARY
JNIEnv* SDL_AndroidGetJNIEnv();
#endif
EXPORT u32* osal_GetCameraImage(int cam_id, int* pw,int* ph){
	JNIEnv* env=(JNIEnv*)SDL_AndroidGetJNIEnv();
	TCamera* cam=&g_cameras[cam_id];
	if((unsigned int)cam_id>=(unsigned int)MAX_CAMERAS||!g_cameras[cam_id].m_is_on)return NULL;
	SDL_LockMutex(cam->m_cam_mutex);
	if(cam->m_image_ready){
		u32* ret=cam->m_image_back;
		cam->m_image_back=cam->m_image_front;
		cam->m_image_front=ret;
		cam->m_image_ready=0;
		SDL_UnlockMutex(cam->m_cam_mutex);
		*pw=cam->m_w;
		*ph=cam->m_h;
		return ret;
	}else{
		SDL_UnlockMutex(cam->m_cam_mutex);
		return NULL;
	}
}

//main thread only
EXPORT int osal_TurnOnCamera(int cam_id,int w,int h,int fps){
	JNIEnv* env=(JNIEnv*)SDL_AndroidGetJNIEnv();
	TCamera* cam=&g_cameras[cam_id];
	jmethodID method_id;
	jvalue args[6];
	int ret;
	jclass clazz=(*env)->FindClass(env,"com/spap/wrapper/camera");
	if((unsigned int)cam_id>=(unsigned int)MAX_CAMERAS)return 0;
	if(!cam->m_inited){
		method_id=(*env)->GetMethodID(env,clazz,"<init>","()V");
		cam->m_cam_object=(jobject)(*env)->NewGlobalRef(env,(*env)->NewObjectA(env,clazz,method_id,NULL));
		cam->m_cam_mutex=SDL_CreateMutex();
		cam->m_inited=1;
	}
	if(cam->m_is_on)return 1;
	SDL_LockMutex(cam->m_cam_mutex);
	cam->m_is_on=1;
	method_id=(*env)->GetMethodID(env,clazz,"turn_on","(IIIIII)I");
	//args[].l=cam->m_cam_object;
	args[0].i=cam_id;
	args[1].i=w;
	args[2].i=h;
	args[3].i=fps;
	args[4].i=cam->m_android_is_texid_valid?cam->m_android_texid:1;
	args[5].i=cam->m_android_is_texid_valid;
	ret=(*env)->CallIntMethodA(env,cam->m_cam_object,method_id,args);
	SDL_UnlockMutex(cam->m_cam_mutex);
	//__android_log_print(ANDROID_LOG_ERROR,"STDOUT","call method %p %p ret=%d",cam->m_clazz,method_id,ret);
	return ret;
}

EXPORT int osal_TurnOffCamera(int cam_id){
	JNIEnv* env=(JNIEnv*)SDL_AndroidGetJNIEnv();
	TCamera* cam=&g_cameras[cam_id];
	jmethodID method_id;
	jvalue args[1];
	int ret;
	jclass clazz=(*env)->FindClass(env,"com/spap/wrapper/camera");
	if((unsigned int)cam_id>=(unsigned int)MAX_CAMERAS)return 0;
	if(!cam->m_is_on)return 1;
	SDL_LockMutex(cam->m_cam_mutex);
	method_id=(*env)->GetMethodID(env,clazz,"turn_off","()I");
	args[0].l=NULL;
	ret=(*env)->CallIntMethodA(env,cam->m_cam_object,method_id,args);
	if(cam->m_image_back){
		free(cam->m_image_back);
		cam->m_image_back=NULL;
	}
	if(cam->m_image_front){
		free(cam->m_image_front);
		cam->m_image_front=NULL;
	}
	cam->m_is_on=0;
	SDL_UnlockMutex(cam->m_cam_mutex);
	return ret;
}

EXPORT int osal_GetFrontCameraId(){
	jmethodID method_id;
	JNIEnv* env=(JNIEnv*)SDL_AndroidGetJNIEnv();
	jclass clazz=(*env)->FindClass(env,"com/spap/wrapper/camera");
	method_id=(*env)->GetStaticMethodID(env,clazz,"get_front_camera_id","()I");
	return (*env)->CallStaticIntMethodA(env,clazz,method_id,NULL);
}

EXPORT int osal_GetBackCameraId(){
	jmethodID method_id;
	JNIEnv* env=(JNIEnv*)SDL_AndroidGetJNIEnv();
	jclass clazz=(*env)->FindClass(env,"com/spap/wrapper/camera");
	method_id=(*env)->GetStaticMethodID(env,clazz,"get_back_camera_id","()I");
	return (*env)->CallStaticIntMethodA(env,clazz,method_id,NULL);
}

EXPORT void osal_AndroidSetCameraPreviewTexture(int cam_id,int texid){
	TCamera* cam=&g_cameras[cam_id];
	cam->m_android_texid=texid;
	cam->m_android_is_texid_valid=1;
}

EXPORT int osal_AndroidCallUpdateTexImage(int cam_id){
	JNIEnv* env=(JNIEnv*)SDL_AndroidGetJNIEnv();
	TCamera* cam=&g_cameras[cam_id];
	jmethodID method_id;
	jvalue args[1];
	int ret;
	jclass clazz=(*env)->FindClass(env,"com/spap/wrapper/camera");
	if((unsigned int)cam_id>=(unsigned int)MAX_CAMERAS){return 0;}
	if(!cam->m_is_on){return 0;}
	method_id=(*env)->GetMethodID(env,clazz,"callUpdateTexImage","()I");
	args[0].l=NULL;
	ret=(*env)->CallIntMethodA(env,cam->m_cam_object,method_id,args);
	return ret;
}