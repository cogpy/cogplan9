/*
 * Tensor Logic for Plan9Cog
 * Based on "Tensor Logic: The Language of AI" (arXiv:2510.12269)
 *
 * Unifies deep learning and symbolic AI through tensor equations
 * that map logical rules to Einstein summation operations.
 *
 * Core insight: Relations in logic programming correspond to sparse
 * Boolean tensors, and Datalog rules are einsums over these tensors.
 */

#ifndef _TENSORLOGIC_H_
#define _TENSORLOGIC_H_ 1

#pragma src "/sys/src/libtensorlogic"
#pragma lib "libtensorlogic.a"

/* ============================================================
 * Tensor Types and Structures
 * ============================================================ */

/* Tensor element types */
enum {
	TensorFloat = 0,	/* 32-bit float */
	TensorDouble,		/* 64-bit double */
	TensorInt,		/* 32-bit integer */
	TensorBool,		/* Boolean (0/1) */
	TensorComplex,		/* Complex number */
};

/* Tensor storage format */
enum {
	TensorDense = 0,	/* Dense storage */
	TensorSparse,		/* Sparse (COO format) */
	TensorCSR,		/* Compressed Sparse Row */
	TensorTucker,		/* Tucker decomposition */
};

/* Maximum tensor rank */
#define TENSOR_MAX_RANK 8

/* Tensor structure */
typedef struct Tensor Tensor;
struct Tensor {
	int type;		/* Element type */
	int format;		/* Storage format */
	int rank;		/* Number of dimensions */
	int shape[TENSOR_MAX_RANK];	/* Size in each dimension */
	int strides[TENSOR_MAX_RANK];	/* Memory strides */
	ulong nelems;		/* Total elements */
	void *data;		/* Element data */

	/* Sparse storage (COO format) */
	int *indices;		/* Index arrays for sparse */
	int nnz;		/* Number of non-zeros */

	/* Tucker decomposition */
	Tensor *core;		/* Core tensor */
	Tensor **factors;	/* Factor matrices */
	int nfactors;		/* Number of factors */

	char *name;		/* Optional tensor name */
	Lock;			/* Thread safety */
};

/* Tensor creation and memory */
Tensor*	tensorcreate(int type, int rank, int *shape);
Tensor*	tensorcreatef(int rank, ...);	/* Variadic shape */
Tensor*	tensorzeros(int rank, int *shape);
Tensor*	tensorones(int rank, int *shape);
Tensor*	tensoreye(int n);		/* Identity matrix */
Tensor*	tensorrand(int rank, int *shape);
Tensor*	tensorsparse(int type, int rank, int *shape, int nnz);
void	tensorfree(Tensor *t);
Tensor*	tensorcopy(Tensor *t);
Tensor*	tensorreshape(Tensor *t, int rank, int *shape);
Tensor*	tensorview(Tensor *t, int start, int end);

/* Element access */
float	tensorgetf(Tensor *t, ...);	/* Get float element */
void	tensorsetf(Tensor *t, float val, ...);
double	tensorgetd(Tensor *t, ...);
void	tensorsetd(Tensor *t, double val, ...);
int	tensorgeti(Tensor *t, ...);
void	tensorseti(Tensor *t, int val, ...);

/* Bulk operations */
void	tensorfill(Tensor *t, float val);
void	tensorcopydata(Tensor *dst, Tensor *src);

/* ============================================================
 * Tensor Operations (Core Math)
 * ============================================================ */

/* Basic arithmetic */
Tensor*	tensoradd(Tensor *a, Tensor *b);	/* C = A + B */
Tensor*	tensorsub(Tensor *a, Tensor *b);	/* C = A - B */
Tensor*	tensormul(Tensor *a, Tensor *b);	/* C = A * B (element-wise) */
Tensor*	tensordiv(Tensor *a, Tensor *b);	/* C = A / B */
Tensor*	tensorscale(Tensor *t, float s);	/* C = s * A */
Tensor*	tensorneg(Tensor *t);			/* C = -A */

/* Matrix operations */
Tensor*	tensormatmul(Tensor *a, Tensor *b);	/* C = A @ B */
Tensor*	tensortranspose(Tensor *t);		/* Transpose */
Tensor*	tensortransposedim(Tensor *t, int d1, int d2);
Tensor*	tensorinverse(Tensor *t);		/* Matrix inverse */
float	tensordeterminant(Tensor *t);		/* Determinant */

/* Tensor products */
Tensor*	tensorouter(Tensor *a, Tensor *b);	/* Outer product */
Tensor*	tensorkron(Tensor *a, Tensor *b);	/* Kronecker product */

/* Reductions */
float	tensorsum(Tensor *t);			/* Sum all elements */
Tensor*	tensorsumaxis(Tensor *t, int axis);	/* Sum along axis */
float	tensormean(Tensor *t);			/* Mean */
float	tensormax(Tensor *t);			/* Maximum */
float	tensormin(Tensor *t);			/* Minimum */
Tensor*	tensorargmax(Tensor *t, int axis);	/* Argmax */

/* Norms */
float	tensornorm(Tensor *t);			/* L2 norm */
float	tensornormp(Tensor *t, float p);	/* Lp norm */
float	tensorfrobenius(Tensor *t);		/* Frobenius norm */

/* ============================================================
 * Einstein Summation (einsum)
 * ============================================================ */

/*
 * Einstein Summation Notation
 *
 * Format: "indices_A,indices_B->indices_C"
 * Example: "ij,jk->ik" for matrix multiplication
 *
 * Repeated indices are summed over (implicit summation)
 * Indices not in output are projected out
 */

Tensor*	tensoreinsum(char *equation, Tensor *a, Tensor *b);
Tensor*	tensoreinsum3(char *equation, Tensor *a, Tensor *b, Tensor *c);
Tensor*	tensoreinsumv(char *equation, Tensor **tensors, int n);

/* Common einsum patterns */
Tensor*	tensorcontract(Tensor *a, Tensor *b, int *axes_a, int *axes_b, int naxes);
Tensor*	tensortrace(Tensor *t);		/* Trace (sum of diagonal) */
Tensor*	tensordiag(Tensor *t);			/* Extract diagonal */

/* ============================================================
 * Tensor Equation System
 * ============================================================ */

/*
 * Tensor Equation: LHS = f(RHS)
 *
 * LHS: computed tensor with indices
 * RHS: tensor joins and projections
 * f: optional nonlinearity
 */

/* Nonlinearity types */
enum {
	TensorNoOp = 0,		/* No operation */
	TensorStep,		/* Heaviside step: H(x) = x > 0 ? 1 : 0 */
	TensorSigmoid,		/* Sigmoid: 1/(1+exp(-x)) */
	TensorTanh,		/* Hyperbolic tangent */
	TensorRelu,		/* ReLU: max(0, x) */
	TensorSoftmax,		/* Softmax (requires axis) */
	TensorExp,		/* Exponential */
	TensorLog,		/* Natural logarithm */
	TensorSqrt,		/* Square root */
	TensorSin,		/* Sine */
	TensorCos,		/* Cosine */
};

/* Tensor equation structure */
typedef struct TensorEquation TensorEquation;
struct TensorEquation {
	char *lhs;		/* Left-hand side (output indices) */
	char *rhs;		/* Right-hand side (expression) */
	int nonlinearity;	/* Applied nonlinearity */
	float temperature;	/* Temperature for softmax/sigmoid */

	/* Parsed components */
	char **terms;		/* RHS tensor names */
	char **termidx;		/* RHS index strings */
	int nterms;		/* Number of terms */

	Tensor *result;		/* Computed result */
};

/* Equation creation and execution */
TensorEquation*	tenseqcreate(char *lhs, char *rhs, int nonlin);
void		tenseqfree(TensorEquation *eq);
Tensor*		tenseqeval(TensorEquation *eq, Tensor **inputs, int ninputs);

/* Apply nonlinearities */
Tensor*	tensorapply(Tensor *t, int nonlin);
Tensor*	tensorstep(Tensor *t);
Tensor*	tensorsigmoid(Tensor *t);
Tensor*	tensorsigmoidt(Tensor *t, float temp);	/* With temperature */
Tensor*	tensortanh(Tensor *t);
Tensor*	tensorrelu(Tensor *t);
Tensor*	tensorsoftmax(Tensor *t, int axis);
Tensor*	tensorsoftmaxt(Tensor *t, int axis, float temp);

/* ============================================================
 * Logic-to-Tensor Mapping
 * ============================================================ */

/*
 * Datalog Rule -> Tensor Equation
 *
 * Example: Aunt(x,z) ← Sister(x,y), Parent(y,z)
 * Becomes: A[x,z] = step(S[x,y] * P[y,z])
 *
 * Relations become Boolean tensors
 * Joins become Einstein summation
 * Projections become index elimination
 */

/* Relation (Boolean tensor) */
typedef struct TensorRelation TensorRelation;
struct TensorRelation {
	char *name;		/* Relation name */
	int arity;		/* Number of arguments */
	char **argnames;	/* Argument names */
	int *domainsizes;	/* Domain size for each arg */
	Tensor *tensor;		/* Underlying Boolean tensor */
};

/* Datalog rule */
typedef struct TensorRule TensorRule;
struct TensorRule {
	TensorRelation *head;	/* Conclusion */
	TensorRelation **body;	/* Premises */
	int nbody;		/* Number of premises */
	TensorEquation *equation;	/* Generated equation */
};

/* Logic program */
typedef struct TensorProgram TensorProgram;
struct TensorProgram {
	TensorRelation **relations;	/* Relations */
	int nrelations;
	TensorRule **rules;		/* Rules */
	int nrules;
	int maxiterations;		/* Forward chaining limit */
};

/* Relation operations */
TensorRelation*	tensorrelcreate(char *name, int arity, int *sizes);
void		tensorrelfree(TensorRelation *rel);
void		tensorrelassert(TensorRelation *rel, ...);	/* Add tuple */
int		tensorrelquery(TensorRelation *rel, ...);	/* Check tuple */
Tensor*		tensorrelget(TensorRelation *rel);

/* Rule operations */
TensorRule*	tensorrulecreate(TensorRelation *head, TensorRelation **body, int n);
void		tensorrulefree(TensorRule *rule);
void		tensorrulecompile(TensorRule *rule);	/* Generate equation */

/* Program operations */
TensorProgram*	tensorproginit(void);
void		tensorprogfree(TensorProgram *prog);
void		tensorprogaddrule(TensorProgram *prog, TensorRule *rule);
void		tensorprogforward(TensorProgram *prog);	/* Forward chaining */
Tensor*		tensorprogquery(TensorProgram *prog, TensorRelation *rel);

/* ============================================================
 * Embedding Space Reasoning
 * ============================================================ */

/*
 * Sound Reasoning in Embedding Space
 *
 * Objects stored as rows in embedding matrix
 * Relations become tensor products of embeddings
 * Enables analogical reasoning with learned similarities
 */

/* Embedding matrix */
typedef struct TensorEmbedding TensorEmbedding;
struct TensorEmbedding {
	int nobjects;		/* Number of objects */
	int ndims;		/* Embedding dimensions */
	Tensor *matrix;		/* Embedding matrix [nobjects, ndims] */
	char **names;		/* Object names */
};

/* Embedded relation */
typedef struct TensorEmbRel TensorEmbRel;
struct TensorEmbRel {
	char *name;		/* Relation name */
	int arity;		/* Number of arguments */
	TensorEmbedding *embedding;	/* Source embedding */
	Tensor *tensor;		/* Embedded relation tensor */
};

/* Embedding operations */
TensorEmbedding*	tensorembcreate(int nobjects, int ndims);
void			tensorembfree(TensorEmbedding *emb);
void			tensorembset(TensorEmbedding *emb, int idx, Tensor *vec);
void			tensorembsetname(TensorEmbedding *emb, int idx, char *name);
Tensor*			tensorembget(TensorEmbedding *emb, int idx);
int			tensorembfind(TensorEmbedding *emb, char *name);

/* Embedding similarity */
Tensor*		tensorembsim(TensorEmbedding *emb);	/* Gram matrix */
float		tensorembdist(TensorEmbedding *emb, int i, int j);

/* Embedded relation operations */
TensorEmbRel*	tensorembrelcreate(char *name, int arity, TensorEmbedding *emb);
void		tensorembrelfree(TensorEmbRel *rel);
void		tensorembrelassert(TensorEmbRel *rel, int *indices);
float		tensorembrelquery(TensorEmbRel *rel, int *indices);

/* Analogical reasoning */
Tensor*		tensoranalogical(TensorEmbedding *emb, TensorEmbRel *rel,
			int *query, float temperature);

/* ============================================================
 * Kernel Machines
 * ============================================================ */

/*
 * Kernel Machine: Y[Q] = f(A[i]Y[i]K[Q,i] + B)
 *
 * Q: query example index
 * i: support vector index
 * K: Gram matrix (kernel evaluations)
 */

/* Kernel types */
enum {
	KernelLinear = 0,	/* K(x,y) = x·y */
	KernelPolynomial,	/* K(x,y) = (x·y + c)^d */
	KernelGaussian,		/* K(x,y) = exp(-||x-y||²/2σ²) */
	KernelLaplacian,	/* K(x,y) = exp(-||x-y||/σ) */
	KernelSigmoid,		/* K(x,y) = tanh(αx·y + c) */
};

typedef struct TensorKernel TensorKernel;
struct TensorKernel {
	int type;		/* Kernel type */
	float degree;		/* Polynomial degree */
	float sigma;		/* Gaussian sigma */
	float coef;		/* Coefficient */
};

typedef struct TensorKernelMachine TensorKernelMachine;
struct TensorKernelMachine {
	TensorKernel *kernel;	/* Kernel function */
	Tensor *support;	/* Support vectors [n, d] */
	Tensor *alpha;		/* Weights [n] */
	float bias;		/* Bias term */
	int nsupport;		/* Number of support vectors */
};

/* Kernel operations */
TensorKernel*	tensorkernelcreate(int type);
void		tensorkernelfree(TensorKernel *k);
float		tensorkerneleval(TensorKernel *k, Tensor *x, Tensor *y);
Tensor*		tensorkernelgram(TensorKernel *k, Tensor *X);	/* Gram matrix */

/* Kernel machine operations */
TensorKernelMachine*	tensorkminit(TensorKernel *kernel);
void			tensorkmfree(TensorKernelMachine *km);
void			tensorkmfit(TensorKernelMachine *km, Tensor *X, Tensor *Y);
Tensor*			tensorkmpredict(TensorKernelMachine *km, Tensor *X);

/* ============================================================
 * Graphical Models
 * ============================================================ */

/*
 * Factor Graphs -> Tensor Networks
 *
 * Factors become tensors
 * Marginalization = projection
 * Variable elimination = contraction
 */

typedef struct TensorFactor TensorFactor;
struct TensorFactor {
	char *name;		/* Factor name */
	int *vars;		/* Variable indices */
	int nvars;		/* Number of variables */
	Tensor *values;		/* Factor values */
};

typedef struct TensorFactorGraph TensorFactorGraph;
struct TensorFactorGraph {
	TensorFactor **factors;	/* Factors */
	int nfactors;
	int *varsizes;		/* Variable domain sizes */
	int nvars;		/* Number of variables */
	char **varnames;	/* Variable names */
};

/* Factor operations */
TensorFactor*	tensorfactorcreate(char *name, int *vars, int nvars, int *sizes);
void		tensorfactorfree(TensorFactor *f);
void		tensorfactorset(TensorFactor *f, int *indices, float val);
float		tensorfactorget(TensorFactor *f, int *indices);

/* Factor graph operations */
TensorFactorGraph*	tensorfginit(int nvars, int *sizes);
void			tensorfgfree(TensorFactorGraph *fg);
void			tensorfgaddfactor(TensorFactorGraph *fg, TensorFactor *f);

/* Inference */
Tensor*		tensorfgmarginal(TensorFactorGraph *fg, int var);
float		tensorfgpartition(TensorFactorGraph *fg);	/* Partition function */
float		tensorfgprob(TensorFactorGraph *fg, int *evidence, int nevidence,
				int *query, int nquery);
Tensor*		tensorfgmap(TensorFactorGraph *fg);	/* MAP inference */

/* Message passing */
void		tensorfgbp(TensorFactorGraph *fg, int iterations);	/* Belief propagation */
Tensor*		tensorfgbeliefs(TensorFactorGraph *fg, int var);

/* ============================================================
 * Automatic Differentiation
 * ============================================================ */

/*
 * Gradient computation for tensor equations
 * Enables learning tensor parameters
 */

typedef struct TensorGrad TensorGrad;
struct TensorGrad {
	Tensor *value;		/* Forward value */
	Tensor *grad;		/* Gradient */
	TensorGrad **inputs;	/* Input gradients */
	int ninputs;
	int (*backward)(TensorGrad *self);	/* Backward function */
	void *ctx;		/* Context for backward */
};

/* Gradient operations */
TensorGrad*	tensorgradinit(Tensor *t);
void		tensorgradfree(TensorGrad *g);
void		tensorgradbackward(TensorGrad *g);
Tensor*		tensorgradget(TensorGrad *g);

/* Gradient-enabled operations */
TensorGrad*	tensorgradmatmul(TensorGrad *a, TensorGrad *b);
TensorGrad*	tensorgradadd(TensorGrad *a, TensorGrad *b);
TensorGrad*	tensorgradmul(TensorGrad *a, TensorGrad *b);
TensorGrad*	tensorgradsoftmax(TensorGrad *a, int axis);
TensorGrad*	tensorgradloss(TensorGrad *pred, Tensor *target);	/* MSE loss */

/* Optimizers */
typedef struct TensorOptimizer TensorOptimizer;
struct TensorOptimizer {
	float lr;		/* Learning rate */
	float momentum;		/* Momentum */
	float decay;		/* Weight decay */
	Tensor **params;	/* Parameters */
	Tensor **velocities;	/* Momentum velocities */
	int nparams;
};

TensorOptimizer*	tensoroptinit(float lr);
void			tensoroptfree(TensorOptimizer *opt);
void			tensoroptaddparam(TensorOptimizer *opt, Tensor *param);
void			tensoroptstep(TensorOptimizer *opt, TensorGrad **grads);
void			tensoroptzerograd(TensorOptimizer *opt);

/* ============================================================
 * Integration with Plan9Cog
 * ============================================================ */

/* Convert AtomSpace relations to tensor relations */
TensorRelation*	atomtotensorrel(struct AtomSpace *as, struct Atom *predicate);
void		tensorreltoadom(TensorRelation *rel, struct AtomSpace *as);

/* Convert PLN rules to tensor rules */
TensorRule*	plntotensorrule(struct PlnInference *pln, struct PlnRule *rule);
void		tensorruletoopln(TensorRule *rule, struct PlnInference *pln);

/* Tensor-enhanced inference */
Tensor*		tensorplnforward(struct PlnInference *pln, int steps);
Tensor*		tensorplnbackward(struct PlnInference *pln, struct Atom *goal);

/* Embedding integration with AtomSpace */
TensorEmbedding*	atomspaceembed(struct AtomSpace *as, int dims);
void			embedtoatomspace(TensorEmbedding *emb, struct AtomSpace *as);

#endif /* _TENSORLOGIC_H_ */
