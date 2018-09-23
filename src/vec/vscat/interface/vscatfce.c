#include <petsc/private/vecscatterimpl.h> /*I   "petscvec.h"    I*/
#if defined(PETSC_HAVE_CUDA)
#include <../src/vec/vec/impls/seq/seqcuda/cudavecimpl.h>
#endif
/* ------------------------------------------------------------------*/
/*@
   VecScatterGetMerged - Returns true if the scatter is completed in the VecScatterBegin()
      and the VecScatterEnd() does nothing

   Not Collective

   Input Parameter:
.   ctx - scatter context created with VecScatterCreate() or VecScatterCreateWithData()

   Output Parameter:
.   flg - PETSC_TRUE if the VecScatterBegin/End() are all done during the VecScatterBegin()

   Level: developer

.seealso: VecScatterCreateWithData(), VecScatterEnd(), VecScatterBegin()
@*/
PetscErrorCode  VecScatterGetMerged(VecScatter ctx,PetscBool  *flg)
{
  PetscFunctionBegin;
  PetscValidHeaderSpecific(ctx,VEC_SCATTER_CLASSID,1);
  *flg = ctx->beginandendtogether;
  PetscFunctionReturn(0);
}

/*@
   VecScatterBegin - Begins a generalized scatter from one vector to
   another. Complete the scattering phase with VecScatterEnd().

   Neighbor-wise Collective on VecScatter and Vec

   Input Parameters:
+  ctx - scatter context generated by VecScatterCreate() or VecScatterCreateWithData()
.  x - the vector from which we scatter
.  y - the vector to which we scatter
.  addv - either ADD_VALUES or INSERT_VALUES, with INSERT_VALUES mode any location
          not scattered to retains its old value; i.e. the vector is NOT first zeroed.
-  mode - the scattering mode, usually SCATTER_FORWARD.  The available modes are:
    SCATTER_FORWARD or SCATTER_REVERSE


   Level: intermediate

   Options Database: See VecScatterCreate()

   Notes:
   The vectors x and y need not be the same vectors used in the call
   to VecScatterSetData() or VecScatterCreateWithData(), but x must have the same parallel data layout
   as that passed in as the x to VecScatterSetData() or VecScatterCreateWithData(), similarly for the y.
   Most likely they have been obtained from VecDuplicate().

   You cannot change the values in the input vector between the calls to VecScatterBegin()
   and VecScatterEnd().

   If you use SCATTER_REVERSE the two arguments x and y should be reversed, from
   the SCATTER_FORWARD.

   y[iy[i]] = x[ix[i]], for i=0,...,ni-1

   This scatter is far more general than the conventional
   scatter, since it can be a gather or a scatter or a combination,
   depending on the indices ix and iy.  If x is a parallel vector and y
   is sequential, VecScatterBegin() can serve to gather values to a
   single processor.  Similarly, if y is parallel and x sequential, the
   routine can scatter from one processor to many processors.

   Concepts: scatter^between vectors
   Concepts: gather^between vectors

.seealso: VecScatterCreateWithData(), VecScatterEnd()
@*/
PetscErrorCode  VecScatterBegin(VecScatter ctx,Vec x,Vec y,InsertMode addv,ScatterMode mode)
{
  PetscErrorCode ierr;
#if defined(PETSC_USE_DEBUG)
  PetscInt       to_n,from_n;
#endif
  PetscFunctionBegin;
  PetscValidHeaderSpecific(ctx,VEC_SCATTER_CLASSID,1);
  PetscValidHeaderSpecific(x,VEC_CLASSID,2);
  PetscValidHeaderSpecific(y,VEC_CLASSID,3);
  if (ctx->inuse) SETERRQ(PETSC_COMM_SELF,PETSC_ERR_ARG_WRONGSTATE," Scatter ctx already in use");

#if defined(PETSC_USE_DEBUG)
  /*
     Error checking to make sure these vectors match the vectors used
   to create the vector scatter context. -1 in the from_n and to_n indicate the
   vector lengths are unknown (for example with mapped scatters) and thus
   no error checking is performed.
  */
  if (ctx->from_n >= 0 && ctx->to_n >= 0) {
    ierr = VecGetLocalSize(x,&from_n);CHKERRQ(ierr);
    ierr = VecGetLocalSize(y,&to_n);CHKERRQ(ierr);
    if (mode & SCATTER_REVERSE) {
      if (to_n != ctx->from_n) SETERRQ2(PETSC_COMM_SELF,PETSC_ERR_ARG_SIZ,"Vector wrong size %D for scatter %D (scatter reverse and vector to != ctx from size)",to_n,ctx->from_n);
      if (from_n != ctx->to_n) SETERRQ2(PETSC_COMM_SELF,PETSC_ERR_ARG_SIZ,"Vector wrong size %D for scatter %D (scatter reverse and vector from != ctx to size)",from_n,ctx->to_n);
    } else {
      if (to_n != ctx->to_n)     SETERRQ2(PETSC_COMM_SELF,PETSC_ERR_ARG_SIZ,"Vector wrong size %D for scatter %D (scatter forward and vector to != ctx to size)",to_n,ctx->to_n);
      if (from_n != ctx->from_n) SETERRQ2(PETSC_COMM_SELF,PETSC_ERR_ARG_SIZ,"Vector wrong size %D for scatter %D (scatter forward and vector from != ctx from size)",from_n,ctx->from_n);
    }
  }
#endif

  ctx->inuse = PETSC_TRUE;
  ierr = PetscLogEventBegin(VEC_ScatterBegin,ctx,x,y,0);CHKERRQ(ierr);
  ierr = (*ctx->ops->begin)(ctx,x,y,addv,mode);CHKERRQ(ierr);
  if (ctx->beginandendtogether && ctx->ops->end) {
    ctx->inuse = PETSC_FALSE;
    ierr = (*ctx->ops->end)(ctx,x,y,addv,mode);CHKERRQ(ierr);
  }
  ierr = PetscLogEventEnd(VEC_ScatterBegin,ctx,x,y,0);CHKERRQ(ierr);
  PetscFunctionReturn(0);
}

/* --------------------------------------------------------------------*/
/*@
   VecScatterEnd - Ends a generalized scatter from one vector to another.  Call
   after first calling VecScatterBegin().

   Neighbor-wise Collective on VecScatter and Vec

   Input Parameters:
+  ctx - scatter context generated by VecScatterCreateWithData()
.  x - the vector from which we scatter
.  y - the vector to which we scatter
.  addv - either ADD_VALUES or INSERT_VALUES.
-  mode - the scattering mode, usually SCATTER_FORWARD.  The available modes are:
     SCATTER_FORWARD, SCATTER_REVERSE

   Level: intermediate

   Notes:
   If you use SCATTER_REVERSE the arguments x and y should be reversed, from the SCATTER_FORWARD.

   y[iy[i]] = x[ix[i]], for i=0,...,ni-1

.seealso: VecScatterBegin(), VecScatterCreateWithData()
@*/
PetscErrorCode  VecScatterEnd(VecScatter ctx,Vec x,Vec y,InsertMode addv,ScatterMode mode)
{
  PetscErrorCode ierr;

  PetscFunctionBegin;
  PetscValidHeaderSpecific(ctx,VEC_SCATTER_CLASSID,1);
  PetscValidHeaderSpecific(x,VEC_CLASSID,2);
  PetscValidHeaderSpecific(y,VEC_CLASSID,3);
  ctx->inuse = PETSC_FALSE;
  if (!ctx->ops->end) PetscFunctionReturn(0);
  if (!ctx->beginandendtogether) {
    ierr = PetscLogEventBegin(VEC_ScatterEnd,ctx,x,y,0);CHKERRQ(ierr);
    ierr = (*(ctx)->ops->end)(ctx,x,y,addv,mode);CHKERRQ(ierr);
    ierr = PetscLogEventEnd(VEC_ScatterEnd,ctx,x,y,0);CHKERRQ(ierr);
  }
  PetscFunctionReturn(0);
}

/*@
   VecScatterDestroy - Destroys a scatter context created by
   VecScatterCreate() or VecScatterCreateWithData()

   Collective on VecScatter

   Input Parameter:
.  ctx - the scatter context

   Level: intermediate

.seealso: VecScatterCreateWithData(), VecScatterCopy()
@*/
PetscErrorCode VecScatterDestroy(VecScatter *ctx)
{
  PetscErrorCode ierr;

  PetscFunctionBegin;
  if (!*ctx) PetscFunctionReturn(0);
  PetscValidHeaderSpecific(*ctx,VEC_SCATTER_CLASSID,1);
  if ((*ctx)->inuse && ((PetscObject)(*ctx))->refct == 1) SETERRQ(((PetscObject)(*ctx))->comm,PETSC_ERR_ARG_WRONGSTATE,"Scatter context is in use");
  if (--((PetscObject)(*ctx))->refct > 0) {*ctx = 0; PetscFunctionReturn(0);}

  /* if memory was published with SAWs then destroy it */
  ierr = PetscObjectSAWsViewOff((PetscObject)(*ctx));CHKERRQ(ierr);
  if ((*ctx)->ops->destroy) {ierr = (*(*ctx)->ops->destroy)(*ctx);CHKERRQ(ierr);}
#if defined(PETSC_HAVE_CUDA)
  ierr = VecScatterCUDAIndicesDestroy((PetscCUDAIndices*)&((*ctx)->spptr));CHKERRQ(ierr);
#endif
  ierr = PetscHeaderDestroy(ctx);CHKERRQ(ierr);
  PetscFunctionReturn(0);
}

/*@
   VecScatterSetUp - Sets up the VecScatter to be able to actually scatter information between vectors

   Collective on VecScatter

   Input Parameter:
.  ctx - the scatter context

   Level: intermediate

.seealso: VecScatterCreateWithData(), VecScatterCopy()
@*/
PetscErrorCode VecScatterSetUp(VecScatter ctx)
{
  PetscErrorCode ierr;

  PetscFunctionBegin;
  PetscValidHeaderSpecific(ctx,VEC_SCATTER_CLASSID,1);
  ierr = (*ctx->ops->setup)(ctx);CHKERRQ(ierr);
  PetscFunctionReturn(0);
}

/*@
   VecScatterCopy - Makes a copy of a scatter context.

   Collective on VecScatter

   Input Parameter:
.  sctx - the scatter context

   Output Parameter:
.  ctx - the context copy

   Level: advanced

.seealso: VecScatterCreateWithData(), VecScatterDestroy()
@*/
PetscErrorCode  VecScatterCopy(VecScatter sctx,VecScatter *ctx)
{
  PetscErrorCode ierr;
  VecScatterType type;

  PetscFunctionBegin;
  PetscValidHeaderSpecific(sctx,VEC_SCATTER_CLASSID,1);
  PetscValidPointer(ctx,2);
  if (!sctx->ops->copy) SETERRQ(PETSC_COMM_SELF,PETSC_ERR_SUP,"Cannot copy this type");
  ierr = PetscHeaderCreate(*ctx,VEC_SCATTER_CLASSID,"VecScatter","VecScatter","Vec",PetscObjectComm((PetscObject)sctx),VecScatterDestroy,VecScatterView);CHKERRQ(ierr);
  (*ctx)->to_n   = sctx->to_n;
  (*ctx)->from_n = sctx->from_n;
  ierr = (*sctx->ops->copy)(sctx,*ctx);CHKERRQ(ierr);

  ierr = VecScatterGetType(sctx,&type);CHKERRQ(ierr);
  ierr = PetscObjectChangeTypeName((PetscObject)(*ctx),type);CHKERRQ(ierr);
  PetscFunctionReturn(0);
}

/* ------------------------------------------------------------------*/
/*@C
   VecScatterView - Views a vector scatter context.

   Collective on VecScatter

   Input Parameters:
+  ctx - the scatter context
-  viewer - the viewer for displaying the context

   Level: intermediate

@*/
PetscErrorCode  VecScatterView(VecScatter ctx,PetscViewer viewer)
{
  PetscErrorCode ierr;

  PetscFunctionBegin;
  PetscValidHeaderSpecific(ctx,VEC_SCATTER_CLASSID,1);
  if (!viewer) {
    ierr = PetscViewerASCIIGetStdout(PetscObjectComm((PetscObject)ctx),&viewer);CHKERRQ(ierr);
  }
  PetscValidHeaderSpecific(viewer,PETSC_VIEWER_CLASSID,2);
  if (ctx->ops->view) {
    ierr = (*ctx->ops->view)(ctx,viewer);CHKERRQ(ierr);
  }
  PetscFunctionReturn(0);
}

/*@
   VecScatterRemap - Remaps the "from" and "to" indices in a
   vector scatter context. FOR EXPERTS ONLY!

   Collective on VecScatter

   Input Parameters:
+  scat    - vector scatter context
.  tomap   - remapping plan for "to" indices (may be NULL).
-  frommap - remapping plan for "from" indices (may be NULL)

   Level: developer

   Notes:
     In the parallel case the todata contains indices from where the data is taken
     (and then sent to others)! The fromdata contains indices from where the received
     data is finally put locally.

     In the sequential case the todata contains indices from where the data is put
     and the fromdata contains indices from where the data is taken from.
     This is backwards from the paralllel case!

@*/
PetscErrorCode  VecScatterRemap(VecScatter scat,PetscInt *tomap,PetscInt *frommap)
{
  VecScatter_MPI_General *to,*from;
  VecScatter_Seq_General *sgto,*sgfrom;
  VecScatter_Seq_Stride  *ssto;
  PetscInt               i,ierr;

  PetscFunctionBegin;
  PetscValidHeaderSpecific(scat,VEC_SCATTER_CLASSID,1);
  if (tomap)   PetscValidIntPointer(tomap,2);
  if (frommap) PetscValidIntPointer(frommap,3);

  to     = (VecScatter_MPI_General*)scat->todata;
  from   = (VecScatter_MPI_General*)scat->fromdata;
  ssto   = (VecScatter_Seq_Stride*)scat->todata;
  sgto   = (VecScatter_Seq_General*)scat->todata;
  sgfrom = (VecScatter_Seq_General*)scat->fromdata;

  /* remap indices from where we take/read data */
  if (tomap) {
    if (to->format == VEC_SCATTER_MPI_TOALL) {
      SETERRQ(PETSC_COMM_SELF,PETSC_ERR_ARG_SIZ,"Not for to all scatter");
    } else if (to->format == VEC_SCATTER_MPI_GENERAL) {
      /* handle off processor parts */
      for (i=0; i<to->starts[to->n]; i++) to->indices[i] = tomap[to->indices[i]];

      /* handle local part */
      for (i=0; i<to->local.n; i++) to->local.vslots[i] = tomap[to->local.vslots[i]];

      /* the memcpy optimizations in vecscatter was based on index patterns it has.
         They need to be recalculated when indices are changed (remapped).
       */
      ierr = VecScatterMemcpyPlanDestroy_PtoP(to,from);CHKERRQ(ierr);
      ierr = VecScatterMemcpyPlanCreate_PtoP(to,from);CHKERRQ(ierr);
    } else if (sgfrom->format == VEC_SCATTER_SEQ_GENERAL) {
      /* remap indices*/
      for (i=0; i<sgfrom->n; i++) sgfrom->vslots[i] = tomap[sgfrom->vslots[i]];
      /* update optimizations, which happen when it is a Stride1toSG, SGtoStride1 or SGToSG vecscatter */
      if (ssto->format == VEC_SCATTER_SEQ_STRIDE && ssto->step == 1) {
        PetscInt tmp[2];
        tmp[0] = 0; tmp[1] = sgfrom->n;
        ierr = VecScatterMemcpyPlanDestroy(&sgfrom->memcpy_plan);CHKERRQ(ierr);
        ierr = VecScatterMemcpyPlanCreate_Index(1,tmp,sgfrom->vslots,1/*bs*/,&sgfrom->memcpy_plan);CHKERRQ(ierr);
      } else if (sgto->format == VEC_SCATTER_SEQ_GENERAL) {
        ierr = VecScatterMemcpyPlanDestroy(&sgto->memcpy_plan);CHKERRQ(ierr);;
        ierr = VecScatterMemcpyPlanDestroy(&sgfrom->memcpy_plan);CHKERRQ(ierr);
        ierr = VecScatterMemcpyPlanCreate_SGToSG(1/*bs*/,sgto,sgfrom);CHKERRQ(ierr);
      }
    } else if (sgfrom->format == VEC_SCATTER_SEQ_STRIDE) {
      VecScatter_Seq_Stride *ssto = (VecScatter_Seq_Stride*)sgfrom;

      /* if the remapping is the identity and stride is identity then skip remap */
      if (ssto->step == 1 && ssto->first == 0) {
        for (i=0; i<ssto->n; i++) {
          if (tomap[i] != i) SETERRQ(PETSC_COMM_SELF,PETSC_ERR_ARG_SIZ,"Unable to remap such scatters");
        }
      } else SETERRQ(PETSC_COMM_SELF,PETSC_ERR_ARG_SIZ,"Unable to remap such scatters");
    } else SETERRQ(PETSC_COMM_SELF,PETSC_ERR_ARG_SIZ,"Unable to remap such scatters");
  }

  if (frommap) SETERRQ(PETSC_COMM_SELF,PETSC_ERR_SUP,"Unable to remap the FROM in scatters yet");

  /*
    Mark then vector lengths as unknown because we do not know the
    lengths of the remapped vectors
  */
  scat->from_n = -1;
  scat->to_n   = -1;
  PetscFunctionReturn(0);
}

/*
 VecScatterGetTypes_Private - Returns the scatter types.

 scatter - The scatter.
 from    - Upon exit this contains the type of the from scatter.
 to      - Upon exit this contains the type of the to scatter.
*/
PetscErrorCode VecScatterGetTypes_Private(VecScatter scatter,VecScatterFormat *from,VecScatterFormat *to)
{
  VecScatter_Common* fromdata = (VecScatter_Common*)scatter->fromdata;
  VecScatter_Common* todata   = (VecScatter_Common*)scatter->todata;

  PetscFunctionBegin;
  *from = fromdata->format;
  *to = todata->format;
  PetscFunctionReturn(0);
}


/*
  VecScatterIsSequential_Private - Returns true if the scatter is sequential.

  scatter - The scatter.
  flag    - Upon exit flag is true if the scatter is of type VecScatter_Seq_General
            or VecScatter_Seq_Stride; otherwise flag is false.
*/
PetscErrorCode VecScatterIsSequential_Private(VecScatter_Common *scatter,PetscBool *flag)
{
  VecScatterFormat scatterType = scatter->format;

  PetscFunctionBegin;
  if (scatterType == VEC_SCATTER_SEQ_GENERAL || scatterType == VEC_SCATTER_SEQ_STRIDE) {
    *flag = PETSC_TRUE;
  } else {
    *flag = PETSC_FALSE;
  }
  PetscFunctionReturn(0);
}

#if defined(PETSC_HAVE_CUDA)

/*@C
   VecScatterInitializeForGPU - Initializes a generalized scatter from one vector
   to another for GPU based computation.

   Input Parameters:
+  inctx - scatter context generated by VecScatterCreateWithData()
.  x - the vector from which we scatter
-  mode - the scattering mode, usually SCATTER_FORWARD.  The available modes are:
    SCATTER_FORWARD or SCATTER_REVERSE

  Level: intermediate

  Notes:
   Effectively, this function creates all the necessary indexing buffers and work
   vectors needed to move data only those data points in a vector which need to
   be communicated across ranks. This is done at the first time this function is
   called. Currently, this only used in the context of the parallel SpMV call in
   MatMult_MPIAIJCUSPARSE.

   This function is executed before the call to MatMult. This enables the memory
   transfers to be overlapped with the MatMult SpMV kernel call.

.seealso: VecScatterFinalizeForGPU(), VecScatterCreateWithData(), VecScatterEnd()
@*/
PETSC_EXTERN PetscErrorCode VecScatterInitializeForGPU(VecScatter inctx,Vec x,ScatterMode mode)
{
  PetscFunctionBegin;
  VecScatter_MPI_General *to,*from;
  PetscErrorCode         ierr;
  PetscInt               i,*indices,*sstartsSends,*sstartsRecvs,nrecvs,nsends,bs;
  PetscBool              isSeq1,isSeq2;

  PetscFunctionBegin;
  ierr = VecScatterIsSequential_Private((VecScatter_Common*)inctx->fromdata,&isSeq1);CHKERRQ(ierr);
  ierr = VecScatterIsSequential_Private((VecScatter_Common*)inctx->todata,&isSeq2);CHKERRQ(ierr);
  if (isSeq1 || isSeq2) {
    PetscFunctionReturn(0);
  }
  if (mode & SCATTER_REVERSE) {
    to     = (VecScatter_MPI_General*)inctx->fromdata;
    from   = (VecScatter_MPI_General*)inctx->todata;
  } else {
    to     = (VecScatter_MPI_General*)inctx->todata;
    from   = (VecScatter_MPI_General*)inctx->fromdata;
  }
  bs           = to->bs;
  nrecvs       = from->n;
  nsends       = to->n;
  indices      = to->indices;
  sstartsSends = to->starts;
  sstartsRecvs = from->starts;
  if (x->valid_GPU_array != PETSC_OFFLOAD_UNALLOCATED && (nsends>0 || nrecvs>0)) {
    if (!inctx->spptr) {
      PetscInt k,*tindicesSends,*sindicesSends,*tindicesRecvs,*sindicesRecvs;
      PetscInt ns = sstartsSends[nsends],nr = sstartsRecvs[nrecvs];
      /* Here we create indices for both the senders and receivers. */
      ierr = PetscMalloc1(ns,&tindicesSends);CHKERRQ(ierr);
      ierr = PetscMalloc1(nr,&tindicesRecvs);CHKERRQ(ierr);

      ierr = PetscMemcpy(tindicesSends,indices,ns*sizeof(PetscInt));CHKERRQ(ierr);
      ierr = PetscMemcpy(tindicesRecvs,from->indices,nr*sizeof(PetscInt));CHKERRQ(ierr);

      ierr = PetscSortRemoveDupsInt(&ns,tindicesSends);CHKERRQ(ierr);
      ierr = PetscSortRemoveDupsInt(&nr,tindicesRecvs);CHKERRQ(ierr);

      ierr = PetscMalloc1(bs*ns,&sindicesSends);CHKERRQ(ierr);
      ierr = PetscMalloc1(from->bs*nr,&sindicesRecvs);CHKERRQ(ierr);

      /* sender indices */
      for (i=0; i<ns; i++) {
        for (k=0; k<bs; k++) sindicesSends[i*bs+k] = tindicesSends[i]+k;
      }
      ierr = PetscFree(tindicesSends);CHKERRQ(ierr);

      /* receiver indices */
      for (i=0; i<nr; i++) {
        for (k=0; k<from->bs; k++) sindicesRecvs[i*from->bs+k] = tindicesRecvs[i]+k;
      }
      ierr = PetscFree(tindicesRecvs);CHKERRQ(ierr);

      /* create GPU indices, work vectors, ... */
      ierr = VecScatterCUDAIndicesCreate_PtoP(ns*bs,sindicesSends,nr*from->bs,sindicesRecvs,(PetscCUDAIndices*)&inctx->spptr);CHKERRQ(ierr);
      ierr = PetscFree(sindicesSends);CHKERRQ(ierr);
      ierr = PetscFree(sindicesRecvs);CHKERRQ(ierr);
    }
  }
  PetscFunctionReturn(0);
}

/*@C
   VecScatterFinalizeForGPU - Finalizes a generalized scatter from one vector to
   another for GPU based computation.

   Input Parameter:
+  inctx - scatter context generated by VecScatterCreateWithData()

  Level: intermediate

  Notes:
   Effectively, this function resets the temporary buffer flags. Currently, this
   only used in the context of the parallel SpMV call in in MatMult_MPIAIJCUDA
   or MatMult_MPIAIJCUDAARSE. Once the MatMultAdd is finished, the GPU temporary
   buffers used for messaging are no longer valid.

.seealso: VecScatterInitializeForGPU(), VecScatterCreateWithData(), VecScatterEnd()
@*/
PETSC_EXTERN PetscErrorCode VecScatterFinalizeForGPU(VecScatter inctx)
{
  PetscFunctionBegin;
  PetscFunctionReturn(0);
}

#endif
