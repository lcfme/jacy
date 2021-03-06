/*
C:\\Program Files (x86)\\Intel\\Composer XE\\mkl\\lib\\intel64
<package name="MKL">
	<target n="win32">
		<dll n="%ProgramFiles(x86)%/Intel/Composer XE/mkl/redist/ia32/mkl/mkl_core.dll"/>
		<lib n="%ProgramFiles(x86)%/Intel/Composer XE/mkl/lib/ia32/mkl_intel_dll.lib"/>
		<lib n="%ProgramFiles(x86)%/Intel/Composer XE/mkl/lib/ia32/mkl_core_dll.lib"/>
		<lib n="%ProgramFiles(x86)%/Intel/Composer XE/mkl/lib/ia32/mkl_intel_thread_dll.lib"/>
	</target>
	<target n="win64">
		<dll n="%ProgramFiles(x86)%/Intel/Composer XE/mkl/redist/intel64/mkl/mkl_core.dll"/>
		<lib n="%ProgramFiles(x86)%/Intel/Composer XE/mkl/lib/intel64/mkl_intel_lp64_dll.lib"/>
		<lib n="%ProgramFiles(x86)%/Intel/Composer XE/mkl/lib/intel64/mkl_core_dll.lib"/>
		<lib n="%ProgramFiles(x86)%/Intel/Composer XE/mkl/lib/intel64/mkl_intel_thread_dll.lib"/>
	</target>
</package>
*/
//#define MKL_ILP64
#include "wrapper_defines.h"
#include "mkl.h"
#include "mkl_lapacke.h"

EXPORT void* createSymmetricSolver(int* p_rows,int n_rows, int* p_columns){
	int nnz=p_rows[n_rows]-1;
	_MKL_DSS_HANDLE_t handle;
	_INTEGER_t error;
	int i;
	MKL_INT v_MKL_DSS_DEFAULTS=MKL_DSS_DEFAULTS;
	MKL_INT v_MKL_DSS_SYMMETRIC=MKL_DSS_SYMMETRIC;
	//MKL_INT v_MKL_DSS_SYMMETRIC_STRUCTURE=MKL_DSS_SYMMETRIC_STRUCTURE;
	MKL_INT v_MKL_DSS_AUTO_ORDER=MKL_DSS_AUTO_ORDER;
	error=dss_create(handle,v_MKL_DSS_DEFAULTS);
	if(error!=MKL_DSS_SUCCESS){return NULL;}
	error=dss_define_structure(handle,v_MKL_DSS_SYMMETRIC,p_rows,n_rows,n_rows, p_columns,nnz);
	//error=dss_define_structure(handle,v_MKL_DSS_SYMMETRIC_STRUCTURE,p_rows,n_rows,n_rows, p_columns,nnz);
	if(error!=MKL_DSS_SUCCESS){dss_delete(handle,v_MKL_DSS_DEFAULTS);return NULL;}
	error=dss_reorder(handle,v_MKL_DSS_AUTO_ORDER,0);
	//error=dss_reorder(handle,v_MKL_DSS_DEFAULTS,0);
	if(error!=MKL_DSS_SUCCESS){dss_delete(handle,v_MKL_DSS_DEFAULTS);return NULL;}
	return (void*)handle;
}

EXPORT void* createSymmetricSolverSingle(int* p_rows,int n_rows, int* p_columns){
	int nnz=p_rows[n_rows]-1;
	_MKL_DSS_HANDLE_t handle;
	_INTEGER_t error;
	int i;
	MKL_INT v_MKL_DSS_DEFAULTS=MKL_DSS_DEFAULTS;
	MKL_INT v_opts=(MKL_DSS_DEFAULTS|MKL_DSS_SINGLE_PRECISION);
	MKL_INT v_MKL_DSS_SYMMETRIC=MKL_DSS_SYMMETRIC;
	//MKL_INT v_MKL_DSS_SYMMETRIC_STRUCTURE=MKL_DSS_SYMMETRIC_STRUCTURE;
	MKL_INT v_MKL_DSS_AUTO_ORDER=MKL_DSS_AUTO_ORDER;
	error=dss_create(handle,v_opts);
	if(error!=MKL_DSS_SUCCESS){return NULL;}
	error=dss_define_structure(handle,v_MKL_DSS_SYMMETRIC,p_rows,n_rows,n_rows, p_columns,nnz);
	//error=dss_define_structure(handle,v_MKL_DSS_SYMMETRIC_STRUCTURE,p_rows,n_rows,n_rows, p_columns,nnz);
	if(error!=MKL_DSS_SUCCESS){dss_delete(handle,v_MKL_DSS_DEFAULTS);return NULL;}
	error=dss_reorder(handle,v_MKL_DSS_AUTO_ORDER,0);
	//error=dss_reorder(handle,v_MKL_DSS_DEFAULTS,0);
	if(error!=MKL_DSS_SUCCESS){dss_delete(handle,v_MKL_DSS_DEFAULTS);return NULL;}
	return (void*)handle;
}

EXPORT void* createNonSymmetricSolver(int* p_rows,int n_rows, int* p_columns){
	int nnz=p_rows[n_rows]-1;
	_MKL_DSS_HANDLE_t handle;
	_INTEGER_t error;
	int i;
	MKL_INT v_MKL_DSS_DEFAULTS=MKL_DSS_DEFAULTS;
	MKL_INT v_MKL_DSS_NON_SYMMETRIC=MKL_DSS_NON_SYMMETRIC;
	MKL_INT v_MKL_DSS_AUTO_ORDER=MKL_DSS_AUTO_ORDER;
	error=dss_create(handle,v_MKL_DSS_DEFAULTS);
	if(error!=MKL_DSS_SUCCESS){return NULL;}
	error=dss_define_structure(handle,v_MKL_DSS_NON_SYMMETRIC,p_rows,n_rows,n_rows, p_columns,nnz);
	if(error!=MKL_DSS_SUCCESS){dss_delete(handle,v_MKL_DSS_DEFAULTS);return NULL;}
	//error=dss_reorder(handle,v_MKL_DSS_AUTO_ORDER,0);
	error=dss_reorder(handle,v_MKL_DSS_DEFAULTS,0);//todo: test
	if(error!=MKL_DSS_SUCCESS){dss_delete(handle,v_MKL_DSS_DEFAULTS);return NULL;}
	return (void*)handle;
}

EXPORT int factorPositiveDefinite(void* handle,double* A){
	MKL_INT v_MKL_DSS_POSITIVE_DEFINITE=MKL_DSS_POSITIVE_DEFINITE;
	_MKL_DSS_HANDLE_t handlex=(_MKL_DSS_HANDLE_t)handle;
	_INTEGER_t error=dss_factor_real(handlex,v_MKL_DSS_POSITIVE_DEFINITE,A);
	if(error!=MKL_DSS_SUCCESS){
		#ifndef NDEBUG
			switch(error){
			case MKL_DSS_SUCCESS:printf("factorPositiveDefinite: MKL_DSS_SUCCESS\n");break;
			case MKL_DSS_STATE_ERR:printf("factorPositiveDefinite: MKL_DSS_STATE_ERR\n");break;
			case MKL_DSS_INVALID_OPTION:printf("factorPositiveDefinite: MKL_DSS_INVALID_OPTION\n");break;
			case MKL_DSS_OPTION_CONFLICT:printf("factorPositiveDefinite: MKL_DSS_OPTION_CONFLICT\n");break;
			case MKL_DSS_VALUES_ERR:printf("factorPositiveDefinite: MKL_DSS_VALUES_ERR\n");break;
			case MKL_DSS_OUT_OF_MEMORY:printf("factorPositiveDefinite: MKL_DSS_OUT_OF_MEMORY\n");break;
			case MKL_DSS_ZERO_PIVOT:printf("factorPositiveDefinite: MKL_DSS_ZERO_PIVOT\n");break;
			case MKL_DSS_FAILURE:printf("factorPositiveDefinite: MKL_DSS_FAILURE\n");break;
			case MKL_DSS_MSG_LVL_ERR:printf("factorPositiveDefinite: MKL_DSS_MSG_LVL_ERR\n");break;
			case MKL_DSS_TERM_LVL_ERR:printf("factorPositiveDefinite: MKL_DSS_TERM_LVL_ERR\n");break;
			case MKL_DSS_OOC_MEM_ERR:printf("factorPositiveDefinite: MKL_DSS_OOC_MEM_ERR\n");break;
			case MKL_DSS_OOC_OC_ERR:printf("factorPositiveDefinite: MKL_DSS_OOC_OC_ERR\n");break;
			case MKL_DSS_OOC_RW_ERR:printf("factorPositiveDefinite: MKL_DSS_OOC_RW_ERR\n");break;
			}
		#endif
		return 0;
	}
	return 1;
}

EXPORT int dssSolve(void* handle, double* x, double* b,int n){
	MKL_INT v_MKL_DSS_DEFAULTS=MKL_DSS_DEFAULTS;
	_MKL_DSS_HANDLE_t handlex=(_MKL_DSS_HANDLE_t)handle;
	_INTEGER_t error=dss_solve_real(handlex,v_MKL_DSS_DEFAULTS,b,n,x);
	if(error!=MKL_DSS_SUCCESS){return 0;}
	return 1;
}

EXPORT int factorIndefinite(void* handle,double* A){
	MKL_INT v_MKL_DSS_INDEFINITE=MKL_DSS_INDEFINITE;
	_MKL_DSS_HANDLE_t handlex=(_MKL_DSS_HANDLE_t)handle;
	_INTEGER_t error=dss_factor_real(handlex,v_MKL_DSS_INDEFINITE,A);
	if(error!=MKL_DSS_SUCCESS){return 0;}
	return 1;
}

EXPORT int deleteSolver(void* handle){
	MKL_INT v_MKL_DSS_DEFAULTS=MKL_DSS_DEFAULTS;
	_MKL_DSS_HANDLE_t handlex=(_MKL_DSS_HANDLE_t)handle;
	_INTEGER_t error=dss_delete(handlex,v_MKL_DSS_DEFAULTS);
	if(error!=MKL_DSS_SUCCESS){return 0;}
	return 1;
}

EXPORT int symmetricSparseMatrixDenseVector(double* y, int* p_rows,int n_rows, int* p_columns, double* A,double* x){
	mkl_dcsrsymv("U", &n_rows, A,p_rows,p_columns, x,y);
	return 1;
}

EXPORT MKL_INT* factorizeDense(double* A,int N){
	MKL_INT m=N,n=N, lda = N;
	MKL_INT* ipiv=(MKL_INT*)memset(malloc(sizeof(MKL_INT)*N),0,sizeof(MKL_INT)*N);
	MKL_INT info=LAPACKE_dgetrf( LAPACK_ROW_MAJOR,m,n,A,lda,ipiv);
	if(info!=0){free(ipiv);return NULL;}
	return ipiv;
}

EXPORT void deleteDenseFactorization(MKL_INT* ipiv){
	free(ipiv);
}

EXPORT int solveDense(MKL_INT* ipiv,double* A,double* b,int N){
	MKL_INT n=N, nrhs = 1, lda = N, ldb=1;
	return LAPACKE_dgetrs( LAPACK_ROW_MAJOR,'N',n,nrhs,A,lda,ipiv,b,ldb);
}

//EXPORT int SVDf(float* A,int N,int M, float* u,float * vt){
//	int m = M, n = N, lda = M, ldu = M, ldvt = N, info, lwork;
//	//float* u=(float*)calloc(1,ldu*M*sizeof(float));
//	//float* vt=(float*)calloc(1,ldvt*N*sizeof(float));
//	float wkopt=0.f;
//	float* work=NULL;
//	int info=0;
//	int lwork=-1;
//	sgesvd( "All", "All", &m, &n, a, &lda, s, u, &ldu, vt, &ldvt, &wkopt, &lwork,
//	        &info );
//	lwork = (int)wkopt;
//	work = (float*)malloc( lwork*sizeof(float) );
//	/* Compute SVD */
//	sgesvd( "All", "All", &m, &n, a, &lda, s, u, &ldu, vt, &ldvt, work, &lwork,
//	        &info );
//	free(work);
//	return info==0;
//}
