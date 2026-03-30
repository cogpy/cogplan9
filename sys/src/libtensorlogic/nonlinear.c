/*
 * Tensor Nonlinearities and Activations
 * Element-wise functions for tensor equations
 */

#include <u.h>
#include <libc.h>
#include <plan9cog/tensorlogic.h>

/* Heaviside step function: H(x) = x > 0 ? 1 : 0 */
Tensor*
tensorstep(Tensor *t)
{
	Tensor *result;
	float *src, *dst;
	ulong i;

	if(t == nil)
		return nil;

	result = tensorcopy(t);
	if(result == nil)
		return nil;

	src = (float*)t->data;
	dst = (float*)result->data;

	for(i = 0; i < t->nelems; i++)
		dst[i] = (src[i] > 0.0) ? 1.0 : 0.0;

	return result;
}

/* Sigmoid: 1 / (1 + exp(-x)) */
Tensor*
tensorsigmoid(Tensor *t)
{
	Tensor *result;
	float *src, *dst;
	ulong i;

	if(t == nil)
		return nil;

	result = tensorcopy(t);
	if(result == nil)
		return nil;

	src = (float*)t->data;
	dst = (float*)result->data;

	for(i = 0; i < t->nelems; i++)
		dst[i] = 1.0 / (1.0 + exp(-src[i]));

	return result;
}

/* Sigmoid with temperature: 1 / (1 + exp(-x/T)) */
Tensor*
tensorsigmoidt(Tensor *t, float temp)
{
	Tensor *result;
	float *src, *dst;
	ulong i;

	if(t == nil)
		return nil;

	if(temp <= 0.0)
		temp = 1.0;

	result = tensorcopy(t);
	if(result == nil)
		return nil;

	src = (float*)t->data;
	dst = (float*)result->data;

	for(i = 0; i < t->nelems; i++)
		dst[i] = 1.0 / (1.0 + exp(-src[i] / temp));

	return result;
}

/* Hyperbolic tangent */
Tensor*
tensortanh(Tensor *t)
{
	Tensor *result;
	float *src, *dst;
	ulong i;

	if(t == nil)
		return nil;

	result = tensorcopy(t);
	if(result == nil)
		return nil;

	src = (float*)t->data;
	dst = (float*)result->data;

	for(i = 0; i < t->nelems; i++)
		dst[i] = tanh(src[i]);

	return result;
}

/* ReLU: max(0, x) */
Tensor*
tensorrelu(Tensor *t)
{
	Tensor *result;
	float *src, *dst;
	ulong i;

	if(t == nil)
		return nil;

	result = tensorcopy(t);
	if(result == nil)
		return nil;

	src = (float*)t->data;
	dst = (float*)result->data;

	for(i = 0; i < t->nelems; i++)
		dst[i] = (src[i] > 0.0) ? src[i] : 0.0;

	return result;
}

/* Softmax along specified axis */
Tensor*
tensorsoftmax(Tensor *t, int axis)
{
	Tensor *result;
	float *src, *dst;
	int i, j, k;
	int outer, inner, axissize;
	float maxval, sum, expval;

	if(t == nil)
		return nil;

	if(axis < 0 || axis >= t->rank)
		axis = t->rank - 1;

	result = tensorcopy(t);
	if(result == nil)
		return nil;

	src = (float*)t->data;
	dst = (float*)result->data;

	/* Calculate dimensions */
	outer = 1;
	for(i = 0; i < axis; i++)
		outer *= t->shape[i];

	axissize = t->shape[axis];

	inner = 1;
	for(i = axis + 1; i < t->rank; i++)
		inner *= t->shape[i];

	/* Apply softmax along axis */
	for(i = 0; i < outer; i++){
		for(k = 0; k < inner; k++){
			/* Find max for numerical stability */
			maxval = src[i * axissize * inner + k];
			for(j = 1; j < axissize; j++){
				float val = src[i * axissize * inner + j * inner + k];
				if(val > maxval)
					maxval = val;
			}

			/* Compute exp and sum */
			sum = 0.0;
			for(j = 0; j < axissize; j++){
				expval = exp(src[i * axissize * inner + j * inner + k] - maxval);
				dst[i * axissize * inner + j * inner + k] = expval;
				sum += expval;
			}

			/* Normalize */
			if(sum > 0.0){
				for(j = 0; j < axissize; j++){
					dst[i * axissize * inner + j * inner + k] /= sum;
				}
			}
		}
	}

	return result;
}

/* Softmax with temperature */
Tensor*
tensorsoftmaxt(Tensor *t, int axis, float temp)
{
	Tensor *scaled, *result;

	if(t == nil)
		return nil;

	if(temp <= 0.0)
		temp = 1.0;

	/* Scale inputs by 1/temperature */
	scaled = tensorscale(t, 1.0 / temp);
	if(scaled == nil)
		return nil;

	result = tensorsoftmax(scaled, axis);
	tensorfree(scaled);

	return result;
}

/* Apply nonlinearity by type */
Tensor*
tensorapply(Tensor *t, int nonlin)
{
	switch(nonlin){
	case TensorNoOp:
		return tensorcopy(t);
	case TensorStep:
		return tensorstep(t);
	case TensorSigmoid:
		return tensorsigmoid(t);
	case TensorTanh:
		return tensortanh(t);
	case TensorRelu:
		return tensorrelu(t);
	case TensorSoftmax:
		return tensorsoftmax(t, -1);
	case TensorExp:
		return tensorexp(t);
	case TensorLog:
		return tensorlog(t);
	case TensorSqrt:
		return tensorsqrt(t);
	case TensorSin:
		return tensorsin(t);
	case TensorCos:
		return tensorcos(t);
	default:
		return tensorcopy(t);
	}
}

/* Exponential */
Tensor*
tensorexp(Tensor *t)
{
	Tensor *result;
	float *src, *dst;
	ulong i;

	if(t == nil)
		return nil;

	result = tensorcopy(t);
	if(result == nil)
		return nil;

	src = (float*)t->data;
	dst = (float*)result->data;

	for(i = 0; i < t->nelems; i++)
		dst[i] = exp(src[i]);

	return result;
}

/* Natural logarithm */
Tensor*
tensorlog(Tensor *t)
{
	Tensor *result;
	float *src, *dst;
	ulong i;

	if(t == nil)
		return nil;

	result = tensorcopy(t);
	if(result == nil)
		return nil;

	src = (float*)t->data;
	dst = (float*)result->data;

	for(i = 0; i < t->nelems; i++){
		if(src[i] > 0.0)
			dst[i] = log(src[i]);
		else
			dst[i] = -1e10;  /* Very negative for log(0) */
	}

	return result;
}

/* Square root */
Tensor*
tensorsqrt(Tensor *t)
{
	Tensor *result;
	float *src, *dst;
	ulong i;

	if(t == nil)
		return nil;

	result = tensorcopy(t);
	if(result == nil)
		return nil;

	src = (float*)t->data;
	dst = (float*)result->data;

	for(i = 0; i < t->nelems; i++){
		if(src[i] >= 0.0)
			dst[i] = sqrt(src[i]);
		else
			dst[i] = 0.0;
	}

	return result;
}

/* Sine */
Tensor*
tensorsin(Tensor *t)
{
	Tensor *result;
	float *src, *dst;
	ulong i;

	if(t == nil)
		return nil;

	result = tensorcopy(t);
	if(result == nil)
		return nil;

	src = (float*)t->data;
	dst = (float*)result->data;

	for(i = 0; i < t->nelems; i++)
		dst[i] = sin(src[i]);

	return result;
}

/* Cosine */
Tensor*
tensorcos(Tensor *t)
{
	Tensor *result;
	float *src, *dst;
	ulong i;

	if(t == nil)
		return nil;

	result = tensorcopy(t);
	if(result == nil)
		return nil;

	src = (float*)t->data;
	dst = (float*)result->data;

	for(i = 0; i < t->nelems; i++)
		dst[i] = cos(src[i]);

	return result;
}

/* Power function */
Tensor*
tensorpow(Tensor *t, float p)
{
	Tensor *result;
	float *src, *dst;
	ulong i;

	if(t == nil)
		return nil;

	result = tensorcopy(t);
	if(result == nil)
		return nil;

	src = (float*)t->data;
	dst = (float*)result->data;

	for(i = 0; i < t->nelems; i++)
		dst[i] = pow(src[i], p);

	return result;
}

/* Leaky ReLU: x if x > 0, alpha*x otherwise */
Tensor*
tensorleakyrelu(Tensor *t, float alpha)
{
	Tensor *result;
	float *src, *dst;
	ulong i;

	if(t == nil)
		return nil;

	result = tensorcopy(t);
	if(result == nil)
		return nil;

	src = (float*)t->data;
	dst = (float*)result->data;

	for(i = 0; i < t->nelems; i++)
		dst[i] = (src[i] > 0.0) ? src[i] : alpha * src[i];

	return result;
}

/* ELU: x if x > 0, alpha*(exp(x)-1) otherwise */
Tensor*
tensorelu(Tensor *t, float alpha)
{
	Tensor *result;
	float *src, *dst;
	ulong i;

	if(t == nil)
		return nil;

	result = tensorcopy(t);
	if(result == nil)
		return nil;

	src = (float*)t->data;
	dst = (float*)result->data;

	for(i = 0; i < t->nelems; i++){
		if(src[i] > 0.0)
			dst[i] = src[i];
		else
			dst[i] = alpha * (exp(src[i]) - 1.0);
	}

	return result;
}

/* GELU (Gaussian Error Linear Unit) */
Tensor*
tensorgelu(Tensor *t)
{
	Tensor *result;
	float *src, *dst;
	ulong i;
	float x, cdf;

	if(t == nil)
		return nil;

	result = tensorcopy(t);
	if(result == nil)
		return nil;

	src = (float*)t->data;
	dst = (float*)result->data;

	/* GELU(x) = x * Phi(x), where Phi is standard normal CDF */
	/* Approximation: 0.5 * x * (1 + tanh(sqrt(2/pi) * (x + 0.044715 * x^3))) */
	for(i = 0; i < t->nelems; i++){
		x = src[i];
		cdf = 0.5 * (1.0 + tanh(0.7978845608 * (x + 0.044715 * x * x * x)));
		dst[i] = x * cdf;
	}

	return result;
}

/* Swish: x * sigmoid(x) */
Tensor*
tensorswish(Tensor *t)
{
	Tensor *result;
	float *src, *dst;
	ulong i;

	if(t == nil)
		return nil;

	result = tensorcopy(t);
	if(result == nil)
		return nil;

	src = (float*)t->data;
	dst = (float*)result->data;

	for(i = 0; i < t->nelems; i++)
		dst[i] = src[i] / (1.0 + exp(-src[i]));

	return result;
}
