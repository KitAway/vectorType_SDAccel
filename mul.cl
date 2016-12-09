/*----------------------------------------------------------------------------
*
* Author:   Liang Ma (liang-ma@polito.it)
*
*----------------------------------------------------------------------------
*/





__kernel void mul(__global float4* a, __global float *b)
{
	int i = get_global_id(0);
	b[i]= a[i].s0*a[i].s1*a[i].s2*a[i].s3;
}


