/*************************************************************************
 *                                                                       *
 *        N  A  S     P A R A L L E L     B E N C H M A R K S  3.3       *
 *                                                                       *
 *                                  D T					 *
 *                                                                       *
 *************************************************************************
 *                                                                       *
 *   This benchmark is part of the NAS Parallel Benchmark 3.3 suite.     *
 *                                                                       *
 *   Permission to use, copy, distribute and modify this software        *
 *   for any purpose with or without fee is hereby granted.  We          *
 *   request, however, that all derived work reference the NAS           *
 *   Parallel Benchmarks 3.3. This software is provided "as is"          *
 *   without express or implied warranty.                                *
 *                                                                       *
 *   Information on NPB 3.3, including the technical report, the         *
 *   original specifications, source code, results and information       *
 *   on how to submit new results, is available at:                      *
 *                                                                       *
 *          http:  www.nas.nasa.gov/Software/NPB                         *
 *                                                                       *
 *   Send comments or suggestions to  npb@nas.nasa.gov                   *
 *   Send bug reports to              npb-bugs@nas.nasa.gov              *
 *                                                                       *
 *         NAS Parallel Benchmarks Group                                 *
 *         NASA Ames Research Center                                     *
 *         Mail Stop: T27A-1                                             *
 *         Moffett Field, CA   94035-1000                                *
 *                                                                       *
 *         E-mail:  npb@nas.nasa.gov                                     *
 *         Fax:     (650) 604-3957                                       *
 *                                                                       *
 *************************************************************************
 *                                                                       *
 *   Author: M. Frumkin							 *						 *
 *                                                                       *
 *************************************************************************/

#include <string>
#include <libdash.h>
#include <iostream>
#include <unistd.h>

using namespace std;

#include "mpi.h"
#include "npbparams.h"
#include "DGraph.h"

#ifndef CLASS
#define CLASS 'S'
#define NUM_PROCS            1
#endif

int      passed_verification;
extern "C" {
  void    timer_clear( int n );
  void    timer_start( int n );
  void    timer_stop( int n );
  double  timer_read( int n );
  double randlc( double *X, double *A );
  void c_print_results( char const  *name,
                        char   class_name,
                        int    n1,
                        int    n2,
                        int    n3,
                        int    niter,
                        int    nprocs_compiled,
                        int    nprocs_total,
                        double t,
                        double mops,
                        char  const  *optype,
                        int    passed_verification,
                        char const  *npbversion,
                        char const  *compiletime,
                        char  const  *mpicc,
                        char const  *clink,
                        char const  *cmpi_lib,
                        char const  *cmpi_inc,
                        char  const *cflags,
                        char const  *clinkflags );



}
int timer_on=0,timers_tot=64;

int verify(char *bmname,double rnm2) {
  double verify_value=0.0;
  double epsilon=1.0E-8;
  char cls=CLASS;
  int verified=-1;
  if (cls != 'U') {
    if(cls=='S') {
      if(strstr(bmname,"BH")) {
        verify_value=30892725.0;
      } else if(strstr(bmname,"WH")) {
        verify_value=67349758.0;
      } else if(strstr(bmname,"SH")) {
        verify_value=58875767.0;
      } else {
        fprintf(stderr,"No such benchmark as %s.\n",bmname);
      }
      verified = 0;
    } else if(cls=='W') {
      if(strstr(bmname,"BH")) {
        verify_value = 4102461.0;
      } else if(strstr(bmname,"WH")) {
        verify_value = 204280762.0;
      } else if(strstr(bmname,"SH")) {
        verify_value = 186944764.0;
      } else {
        fprintf(stderr,"No such benchmark as %s.\n",bmname);
      }
      verified = 0;
    } else if(cls=='A') {
      if(strstr(bmname,"BH")) {
        verify_value = 17809491.0;
      } else if(strstr(bmname,"WH")) {
        verify_value = 1289925229.0;
      } else if(strstr(bmname,"SH")) {
        verify_value = 610856482.0;
      } else {
        fprintf(stderr,"No such benchmark as %s.\n",bmname);
      }
      verified = 0;
    } else if(cls=='B') {
      if(strstr(bmname,"BH")) {
        verify_value = 4317114.0;
      } else if(strstr(bmname,"WH")) {
        verify_value = 7877279917.0;
      } else if(strstr(bmname,"SH")) {
        verify_value = 1836863082.0;
      } else {
        fprintf(stderr,"No such benchmark as %s.\n",bmname);
        verified = 0;
      }
    } else if(cls=='C') {
      if(strstr(bmname,"BH")) {
        verify_value = 0.0;
      } else if(strstr(bmname,"WH")) {
        verify_value = 0.0;
      } else if(strstr(bmname,"SH")) {
        verify_value = 0.0;
      } else {
        fprintf(stderr,"No such benchmark as %s.\n",bmname);
        verified = -1;
      }
    } else if(cls=='D') {
      if(strstr(bmname,"BH")) {
        verify_value = 0.0;
      } else if(strstr(bmname,"WH")) {
        verify_value = 0.0;
      } else if(strstr(bmname,"SH")) {
        verify_value = 0.0;
      } else {
        fprintf(stderr,"No such benchmark as %s.\n",bmname);
      }
      verified = -1;
    } else {
      fprintf(stderr,"No such class as %c.\n",cls);
    }
    fprintf(stderr," %s L2 Norm = %f\n",bmname,rnm2);
    if(verified==-1) {
      fprintf(stderr," No verification was performed.\n");
    } else if( rnm2 - verify_value < epsilon &&
               rnm2 - verify_value > -epsilon) {  /* abs here does not work on ALTIX */
      verified = 1;
      fprintf(stderr," Deviation = %f\n",(rnm2 - verify_value));
    } else {
      verified = 0;
      fprintf(stderr," The correct verification value = %f\n",verify_value);
      fprintf(stderr," Got value = %f\n",rnm2);
    }
  } else {
    verified = -1;
  }
  return  verified;
}

int ipowMod(int a,long long int n,int md) {
  int seed=1,q=a,r=1;
  if(n<0) {
    fprintf(stderr,"ipowMod: exponent must be nonnegative exp=%lld\n",n);
    n=-n; /* temp fix */
    /*    return 1; */
  }
  if(md<=0) {
    fprintf(stderr,"ipowMod: module must be positive mod=%d",md);
    return 1;
  }
  if(n==0) return 1;
  while(n>1) {
    int n2 = n/2;
    if (n2*2==n) {
      seed = (q*q)%md;
      q=seed;
      n = n2;
    } else {
      seed = (r*q)%md;
      r=seed;
      n = n-1;
    }
  }
  seed = (r*q)%md;
  return seed;
}

DGraph *buildSH(char cls) {
  /*
    Nodes of the graph must be topologically sorted
    to avoid MPI deadlock.
  */
  DGraph *dg;
  int numSources=NUM_SOURCES; /* must be power of 2 */
  int numOfLayers=0,tmpS=numSources>>1;
  int firstLayerNode=0;
  DGArc *ar=NULL;
  DGNode *nd=NULL;
  int mask=0x0,ndid=0,ndoff=0;
  int i=0,j=0;
  char nm[BLOCK_SIZE];

  sprintf(nm,"DT_SH.%c",cls);
  dg=newDGraph(nm);

  while(tmpS>1) {
    numOfLayers++;
    tmpS>>=1;
  }
  for(i=0; i<numSources; i++) {
    sprintf(nm,"Source.%d",i);
    nd=newNode(nm);
    AttachNode(dg,nd);
  }
  for(j=0; j<numOfLayers; j++) {
    mask=0x00000001<<j;
    for(i=0; i<numSources; i++) {
      sprintf(nm,"Comparator.%d",(i+j*firstLayerNode));
      nd=newNode(nm);
      AttachNode(dg,nd);
      ndoff=i&(~mask);
      ndid=firstLayerNode+ndoff;
      ar=newArc(dg->node[ndid],nd);
      AttachArc(dg,ar);
      ndoff+=mask;
      ndid=firstLayerNode+ndoff;
      ar=newArc(dg->node[ndid],nd);
      AttachArc(dg,ar);
    }
    firstLayerNode+=numSources;
  }
  mask=0x00000001<<numOfLayers;
  for(i=0; i<numSources; i++) {
    sprintf(nm,"Sink.%d",i);
    nd=newNode(nm);
    AttachNode(dg,nd);
    ndoff=i&(~mask);
    ndid=firstLayerNode+ndoff;
    ar=newArc(dg->node[ndid],nd);
    AttachArc(dg,ar);
    ndoff+=mask;
    ndid=firstLayerNode+ndoff;
    ar=newArc(dg->node[ndid],nd);
    AttachArc(dg,ar);
  }
  return dg;
}
DGraph *buildWH(char cls) {
  /*
    Nodes of the graph must be topologically sorted
    to avoid MPI deadlock.
  */
  int i=0,j=0;
  int numSources=NUM_SOURCES,maxInDeg=4;
  int numLayerNodes=numSources,firstLayerNode=0;
  int totComparators=0;
  int numPrevLayerNodes=numLayerNodes;
  int id=0,sid=0;
  DGraph *dg;
  DGNode *nd=NULL,*source=NULL,*tmp=NULL,*snd=NULL;
  DGArc *ar=NULL;
  char nm[BLOCK_SIZE];

  sprintf(nm,"DT_WH.%c",cls);
  dg=newDGraph(nm);

  for(i=0; i<numSources; i++) {
    sprintf(nm,"Sink.%d",i);
    nd=newNode(nm);
    AttachNode(dg,nd);
  }
  totComparators=0;
  numPrevLayerNodes=numLayerNodes;
  while(numLayerNodes>maxInDeg) {
    numLayerNodes=numLayerNodes/maxInDeg;
    if(numLayerNodes*maxInDeg<numPrevLayerNodes)numLayerNodes++;
    for(i=0; i<numLayerNodes; i++) {
      sprintf(nm,"Comparator.%d",totComparators);
      totComparators++;
      nd=newNode(nm);
      id=AttachNode(dg,nd);
      for(j=0; j<maxInDeg; j++) {
        sid=i*maxInDeg+j;
        if(sid>=numPrevLayerNodes) break;
        snd=dg->node[firstLayerNode+sid];
        ar=newArc(dg->node[id],snd);
        AttachArc(dg,ar);
      }
    }
    firstLayerNode+=numPrevLayerNodes;
    numPrevLayerNodes=numLayerNodes;
  }
  source=newNode("Source");
  AttachNode(dg,source);
  for(i=0; i<numPrevLayerNodes; i++) {
    nd=dg->node[firstLayerNode+i];
    ar=newArc(source,nd);
    AttachArc(dg,ar);
  }

  for(i=0; i<dg->numNodes/2; i++) { /* Topological sorting */
    tmp=dg->node[i];
    dg->node[i]=dg->node[dg->numNodes-1-i];
    dg->node[i]->id=i;
    dg->node[dg->numNodes-1-i]=tmp;
    dg->node[dg->numNodes-1-i]->id=dg->numNodes-1-i;
  }
  return dg;
}
DGraph *buildBH(char cls) {
  /*
    Nodes of the graph must be topologically sorted
    to avoid MPI deadlock.
  */
  int i=0,j=0;
  int numSources=NUM_SOURCES,maxInDeg=4;
  int numLayerNodes=numSources,firstLayerNode=0;
  DGraph *dg;
  DGNode *nd=NULL, *snd=NULL, *sink=NULL;
  DGArc *ar=NULL;
  int totComparators=0;
  int numPrevLayerNodes=numLayerNodes;
  int id=0, sid=0;
  char nm[BLOCK_SIZE];

  sprintf(nm,"DT_BH.%c",cls);
  dg=newDGraph(nm);

  for(i=0; i<numSources; i++) {
    sprintf(nm,"Source.%d",i);
    nd=newNode(nm);

    AttachNode(dg,nd);
  }
  while(numLayerNodes>maxInDeg) {
    numLayerNodes=numLayerNodes/maxInDeg;
    if(numLayerNodes*maxInDeg<numPrevLayerNodes)numLayerNodes++;
    for(i=0; i<numLayerNodes; i++) {
      sprintf(nm,"Comparator.%d",totComparators);
      totComparators++;
      nd=newNode(nm);
      id=AttachNode(dg,nd);
      for(j=0; j<maxInDeg; j++) {
        sid=i*maxInDeg+j;
        if(sid>=numPrevLayerNodes) break;
        snd=dg->node[firstLayerNode+sid];
        ar=newArc(snd,dg->node[id]);
        AttachArc(dg,ar);
      }
    }
    firstLayerNode+=numPrevLayerNodes;
    numPrevLayerNodes=numLayerNodes;
  }
  sink=newNode("Sink");
  AttachNode(dg,sink);
  for(i=0; i<numPrevLayerNodes; i++) {
    nd=dg->node[firstLayerNode+i];
    ar=newArc(nd,sink);
    AttachArc(dg,ar);
  }
  return dg;
}

DGNode_Feat *newFeat(int len) {
  DGNode_Feat *arr=(DGNode_Feat *)malloc(sizeof(DGNode_Feat));
  arr->len=len;
  return arr;
}
void arrShow(DGNode_Feat* a) {
  if(!a) fprintf(stderr,"-- NULL array\n");
  else {
    fprintf(stderr,"-- length=%d\n",a->len);
  }
}
double CheckVal(DGNode_Feat const& feat) {
  double csum=0.0;
  int i=0;
  for(i=0; i<feat.len; i++) {
    csum+=feat.val[i]*feat.val[i]/feat.len; /* The truncation does not work since
                                                  result will be 0 for large len  */
  }
  return csum;
}
int GetFNumDPar(int* mean, int* stdev) {
  *mean=NUM_SAMPLES;
  *stdev=STD_DEVIATION;
  return 0;
}
int GetFeatureNum(char *mbname,int id) {
  double tran=314159265.0;
  double A=2*id+1;
  double denom=randlc(&tran,&A);
  char cval='S';
  int mean=NUM_SAMPLES,stdev=128;
  int rtfs=0,len=0;
  GetFNumDPar(&mean,&stdev);
  rtfs=ipowMod((int)(1/denom)*(int)cval,(long long int) (2*id+1),2*stdev);
  if(rtfs<0) rtfs=-rtfs;
  len=mean-stdev+rtfs;
  return len;
}
void RandomFeatures(char *bmname,int fdim, DGNode& nd) {
  int len=GetFeatureNum(bmname,nd.id)*fdim;
  nd.feat.len=len;
  //cout << "Feature length:  " << nd.feat.len << ", calculated length: " << len << ", Address: " << nd.address << endl;
  int nxg=2,nyg=2,nzg=2,nfg=5;
  int nx=421,ny=419,nz=1427,nf=3527;
  long long int expon=(len*(nd.id+1))%3141592;
  int seedx=ipowMod(nxg,expon,nx),
      seedy=ipowMod(nyg,expon,ny),
      seedz=ipowMod(nzg,expon,nz),
      seedf=ipowMod(nfg,expon,nf);
  int i=0;
  if(timer_on) {
    timer_clear(nd.id+1);
    timer_start(nd.id+1);
  }
  for(i=0; i<len; i+=fdim) {
    seedx=(seedx*nxg)%nx;
    seedy=(seedy*nyg)%ny;
    seedz=(seedz*nzg)%nz;
    seedf=(seedf*nfg)%nf;
    nd.feat.val[i]=seedx;
    nd.feat.val[i+1]=seedy;
    nd.feat.val[i+2]=seedz;
    nd.feat.val[i+3]=seedf;
  }
  if(timer_on) {
    timer_stop(nd.id+1);
    fprintf(stderr,"** RandomFeatures time in node %d = %f\n",nd.id,timer_read(nd.id+1));
  }
}
void Resample(DGNode_Feat *a,int blen) {
  long long int i=0,j=0,jlo=0,jhi=0;
  double avval=0.0;
  double *nval=(double *)malloc(blen*sizeof(double));
  for(i=0; i<blen; i++) nval[i]=0.0;
  for(i=1; i<a->len-1; i++) {
    jlo=(int)(0.5*(2*i-1)*(blen/a->len));
    jhi=(int)(0.5*(2*i+1)*(blen/a->len));

    avval=a->val[i]/(jhi-jlo+1);
    for(j=jlo; j<=jhi; j++) {
      nval[j]+=avval;
    }
  }
  nval[0]=a->val[0];
  nval[blen-1]=a->val[a->len-1];
  memcpy(&(a->val[0]), nval, blen*sizeof(double));
  a->len=blen;
  free(nval);
}
void WindowFilter(DGNode_Feat *a, DGNode_Feat *b,int w) {
  int i=0,j=0,k=0;
  double rms0=0.0,rms1=0.0,rmsm1=0.0;
  double weight=((double) (w+1))/(w+2);

  w+=1;
  if(timer_on) {
    timer_clear(w);
    timer_start(w);
  }
  if(a->len<b->len) Resample(a,b->len);
  if(a->len>b->len) Resample(b,a->len);
  for(i=fielddim; i<a->len-fielddim; i+=fielddim) {
    rms0=(a->val[i]-b->val[i])*(a->val[i]-b->val[i])
         +(a->val[i+1]-b->val[i+1])*(a->val[i+1]-b->val[i+1])
         +(a->val[i+2]-b->val[i+2])*(a->val[i+2]-b->val[i+2])
         +(a->val[i+3]-b->val[i+3])*(a->val[i+3]-b->val[i+3]);
    j=i+fielddim;
    rms1=(a->val[j]-b->val[j])*(a->val[j]-b->val[j])
         +(a->val[j+1]-b->val[j+1])*(a->val[j+1]-b->val[j+1])
         +(a->val[j+2]-b->val[j+2])*(a->val[j+2]-b->val[j+2])
         +(a->val[j+3]-b->val[j+3])*(a->val[j+3]-b->val[j+3]);
    j=i-fielddim;
    rmsm1=(a->val[j]-b->val[j])*(a->val[j]-b->val[j])
          +(a->val[j+1]-b->val[j+1])*(a->val[j+1]-b->val[j+1])
          +(a->val[j+2]-b->val[j+2])*(a->val[j+2]-b->val[j+2])
          +(a->val[j+3]-b->val[j+3])*(a->val[j+3]-b->val[j+3]);
    k=0;
    if(rms1<rms0) {
      k=1;
      rms0=rms1;
    }
    if(rmsm1<rms0) k=-1;
    if(k==0) {
      j=i+fielddim;
      a->val[i]=weight*b->val[i];
      a->val[i+1]=weight*b->val[i+1];
      a->val[i+2]=weight*b->val[i+2];
      a->val[i+3]=weight*b->val[i+3];
    } else if(k==1) {
      j=i+fielddim;
      a->val[i]=weight*b->val[j];
      a->val[i+1]=weight*b->val[j+1];
      a->val[i+2]=weight*b->val[j+2];
      a->val[i+3]=weight*b->val[j+3];
    } else { /*if(k==-1)*/
      j=i-fielddim;
      a->val[i]=weight*b->val[j];
      a->val[i+1]=weight*b->val[j+1];
      a->val[i+2]=weight*b->val[j+2];
      a->val[i+3]=weight*b->val[j+3];
    }
  }
  if(timer_on) {
    timer_stop(w);
    fprintf(stderr,"** WindowFilter time in node %d = %f\n",(w-1),timer_read(w));
  }
}

int SendResults(DGraph *dg, dash::Array<DGNode> & nodes, DGNode const& nd) {
  int i=0;
  for(i=0; i < nd.outDegree; ++i) {
    MPI_Send(&nd.address, 1, MPI_INT, nd.out[i], 10, MPI_COMM_WORLD);
  }
  return 1;
}

void CombineStreams(DGraph *dg, dash::Array<DGNode> const& nodes, DGNode & nd) {

  DGNode_Feat *resfeat = newFeat(NUM_SAMPLES * fielddim);
  int i=0;
  int pred;

  if (nd.inDegree == 0) return;

  DGNode_Feat *feat=NULL;
  MPI_Request *reqs = new MPI_Request[nd.inDegree];
  MPI_Status  *status= new MPI_Status[nd.inDegree];
  //int *indices = new int[nd.inDegree];
  int *results = new int[nd.inDegree];
  //int ncompleted = MPI_UNDEFINED;


  for(i=0; i<nd.inDegree; i++) {
    pred=nd.in[i];
    if(pred != nd.address) {
      MPI_Irecv(&results[i], 1 , MPI_INT, pred , 10 , MPI_COMM_WORLD, &reqs[i]);
    } else {
      feat=newFeat(nd.feat.len);
      memcpy(&(feat->val), &(nd.feat.val), nd.feat.len*sizeof(double));
      WindowFilter(resfeat, feat, nd.id);
      free(feat);
    }
  }

  MPI_Waitall(nd.inDegree, reqs, status);

  for (i = 0; i < nd.inDegree; ++i) {
    DGNode_Feat featp = nodes[results[i]].member(&DGNode_s::feat);
    WindowFilter(resfeat, &featp, nd.id);
  }

  const int inD = nd.inDegree;

  for(i=0; i<resfeat->len; i++) {
    resfeat->val[i]=((int)resfeat->val[i])/inD;
  }


  nd.feat = *resfeat;

  delete[] reqs;
  delete[] status;
  //delete[] indices;
  delete[] results;
}

double Reduce(DGNode_Feat const& a,int w) {
  double retv=0.0;
  if(timer_on) {
    timer_clear(w);
    timer_start(w);
  }
  retv=(int)(w*CheckVal(a));/* The casting needed for node
                               and array dependent verifcation */
  if(timer_on) {
    timer_stop(w);
    fprintf(stderr,"** Reduce time in node %d = %f\n",(w-1),timer_read(w));
  }
  return retv;
}

double ReduceStreams(DGraph *dg, dash::Array<DGNode> const& nodes, DGNode const& nd) {
  double csum=0.0;
  int i=0;
  int pred;
  double retv=0.0;
  MPI_Request *reqs = new MPI_Request[nd.inDegree];
  MPI_Status  *status= new MPI_Status[nd.inDegree];
  //int *indices = new int[nd.inDegree];
  int *results = new int[nd.inDegree];

  for(i=0; i<nd.inDegree; i++) {
    pred=nd.in[i];
    if(pred != nd.address) {
      MPI_Irecv(&results[i], 1 , MPI_INT, pred , 10 , MPI_COMM_WORLD, &reqs[i]);
    } else {
      csum+=Reduce(nd.feat,(nd.id+1));
    }
  }

  MPI_Waitall(nd.inDegree, reqs, status);

  for (i = 0; i < nd.inDegree; ++i) {
    const DGNode_Feat feat = nodes[results[i]].member(&DGNode_s::feat);
    csum+=Reduce(feat,(nd.id+1));
  }
  /*
  if (outstandingRequests) {
  while(1) {
  MPI_Waitsome(nd.inDegree, reqs, &ncompleted, indices, status);

  if (ncompleted == MPI_UNDEFINED) break;

  }
  }
  */
  if(nd.inDegree > 0) csum=(((long long int)csum)/nd.inDegree);
  retv=(nd.id+1)*csum;
  delete[] reqs;
  delete[] status;
  //delete[] indices;
  delete[] results;
  return retv;
}

int ProcessNodes(DGraph *dg, dash::Array<DGNode> (&nodes), int me) {
  double chksum=0.0;
  int verified=0,tag;
  double rchksum=0.0;
  MPI_Status status;
  auto const numNodes = nodes.local.size();
  DGNode &nd = nodes.local[0];

  if (strlen(nd.name) == 0) return 0;

  if(strstr(nd.name,"Source")) {
    RandomFeatures(dg->name,fielddim, nd);
    SendResults(dg, nodes, nd);
  }
  else if(strstr(nd.name,"Sink")) {
    chksum=ReduceStreams(dg, nodes, nd);
    tag = numNodes + nd.id;
    MPI_Send(&chksum,1,MPI_DOUBLE,0,tag,MPI_COMM_WORLD);
  }
  else {
    CombineStreams(dg, nodes, nd);
    SendResults(dg, nodes, nd);
  }

  if(me==0) { // Report node
    rchksum=0.0;
    chksum=0.0;
    DGNode *nd = new DGNode();
    for(size_t idx = 0; idx < nodes.size(); ++idx) {
      nodes[idx].get(nd);
      if(!strstr(nd->name,"Sink")) continue;
      tag=numNodes+nd->id; // make these to avoid clash with arc tags
      MPI_Recv(&rchksum,1,MPI_DOUBLE,nd->address,tag,MPI_COMM_WORLD,&status);
      chksum+=rchksum;
    }
    delete nd;
    verified=verify(dg->name,chksum);
  }

  return verified;
}

int main(int argc,char **argv ) {
  int my_rank,comm_size;
  int i;
  DGraph *dg=NULL;
  int verified=0, featnum=0;
  double bytes_sent=2.0,tot_time=0.0;
  dash::init(&argc, &argv);

  my_rank = dash::myid();
  comm_size = dash::size();

  if(argc!=2||
      (  strncmp(argv[1],"BH",2)!=0
         &&strncmp(argv[1],"WH",2)!=0
         &&strncmp(argv[1],"SH",2)!=0
      )
    ) {
    if(my_rank==0) {
      fprintf(stderr,"** Usage: mpirun -np N ../bin/dt.S GraphName\n");
      fprintf(stderr,"** Where \n   - N is integer number of MPI processes\n");
      fprintf(stderr,"   - S is the class S, W, or A \n");
      fprintf(stderr,"   - GraphName is the communication graph name BH, WH, or SH.\n");
      fprintf(stderr,"   - the number of MPI processes N should not be be less than \n");
      fprintf(stderr,"     the number of nodes in the graph\n");
    }
    dash::finalize();
    exit(0);
  }
  if(strncmp(argv[1],"BH",2)==0) {
    dg=buildBH(CLASS);
  } else if(strncmp(argv[1],"WH",2)==0) {
    dg=buildWH(CLASS);
  } else if(strncmp(argv[1],"SH",2)==0) {
    dg=buildSH(CLASS);
  }
  
  if(dg->numNodes>comm_size) {
    if(my_rank==0) {
      fprintf(stderr,"**  The number of MPI processes should not be less than \n");
      fprintf(stderr,"**  the number of nodes in the graph\n");
      fprintf(stderr,"**  Number of MPI processes = %d\n",comm_size);
      fprintf(stderr,"**  Number nodes in the graph = %d\n",dg->numNodes);
    }
    dash::finalize();
    exit(0);
  }

  if(timer_on&&dg->numNodes+1>timers_tot) {
    timer_on=0;
    if(my_rank==0)
      fprintf(stderr,"Not enough timers. Node timeing is off. \n");
  }

  for(i=0; i<dg->numNodes; i++) {
    dg->node[i]->address=i;
  }

  //Create Dash Array
  dash::Array<DGNode> nodes(dash::size());

  if (my_rank == 0) {
    for(i=0; i<dg->numNodes; i++) {
      int j;
      for (j = 0; j < dg->node[i]->inDegree; ++j)
      {
        dg->node[i]->in[j] = dg->node[i]->inArc[j]->tail->address;
      }
      for (j = 0; j < dg->node[i]->outDegree; ++j)
      {
        dg->node[i]->out[j] = dg->node[i]->outArc[j]->head->address;
      }

      //nodes[i] = *(dg->node[i]);
      nodes[i].put(dg->node[i]);
    }
  }

  nodes.barrier();
  //cout << "Before Graph Show in ID: " << my_rank << endl;

  if( my_rank == 0 ) {
    printf( "\n\n NAS Parallel Benchmarks 3.3 -- DT Benchmark\n\n" );
    graphShow(dg,0);
    timer_clear(0);
    timer_start(0);
  }

  //cout << "After Graph Show" << endl;
  //cout << "After Graph Show in ID: " << my_rank << endl;
  //printf("Starting ProcessNodes %d with dg: %p, nodes size = %d\n", my_rank, (void*) dg, dg->numNodes);

  verified=ProcessNodes(dg, nodes, my_rank);

  nodes.barrier();

  featnum=NUM_SAMPLES*fielddim;
  bytes_sent=featnum*dg->numArcs;
  bytes_sent/=1048576;
  if(my_rank==0) {
    timer_stop(0);
    tot_time=timer_read(0);
    c_print_results( dg->name,
                     CLASS,
                     featnum,
                     0,
                     0,
                     dg->numNodes,
                     0,
                     comm_size,
                     tot_time,
                     bytes_sent/tot_time,
                     "bytes transmitted",
                     verified,
                     NPBVERSION,
                     COMPILETIME,
                     MPICC,
                     CLINK,
                     CMPI_LIB,
                     CMPI_INC,
                     CFLAGS,
                     CLINKFLAGS );
  }
  dash::finalize();
  return 0;
}
