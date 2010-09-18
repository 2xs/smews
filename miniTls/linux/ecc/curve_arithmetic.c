/*
 * Curve Arithmetic Implementation for secp256r1
 */

#include "ecc.h"

static void gfp_aff_to_jac(uint32_t*,uint32_t*,uint32_t*,uint32_t*,uint32_t*);
static void gfp_jac_to_aff(uint32_t*,uint32_t*,uint32_t*);
static void gfp_point_double_jac(uint32_t*,uint32_t*,uint32_t*);
static void gfp_point_add_aff_jac(uint32_t*,uint32_t*,uint32_t*,uint32_t*,uint32_t*);

/* constants one and zero */
extern const uint32_t one[GFP_ARRAY_LENGHT32];
extern const uint32_t zero[GFP_ARRAY_LENGHT32];


/* Converts a point P(px, py) from affine coordinates to Jacobian
 * projective coordinates R(rx, ry, rz). */
static void gfp_aff_to_jac(uint32_t *px,uint32_t *py,uint32_t *rx,uint32_t *ry,uint32_t *rz){


	/* if P is PAI then set R as PAI too */
	if ((gfp_cmp(px,zero) == 0) && (gfp_cmp(py,zero) == 0)) {
		gfp_zeroize(rz);

	} else {
		/* else return (px,py,1) */
		gfp_copy(px,rx);
		gfp_copy(py,ry);
		gfp_copy(one,rz);

	}


}


/* Converts a point P(px, py, pz) from Jacobian projective coordinates to
 * affine coordinates R(px, py) in place. */
static void gfp_jac_to_aff(uint32_t *px,uint32_t *py,uint32_t *pz){

	uint32_t tmp1[GFP_ARRAY_LENGHT32];

	/* if P is PAI then set R to PAI and exit */
	if(gfp_cmp(pz,zero) == 0){
		gfp_zeroize(px);
		gfp_zeroize(py);
		return;

	}

	/* transform (px, py, pz) into (px / pz^2, py / pz^3) */
	if(gfp_cmp(pz,one) == 0){

		return;
	} else {
		gfp_mod_inv(pz); 			/* 1/z */
		gfp_mod_mult(pz,pz,tmp1); 	/* 1/z^2 */
		gfp_mod_mult(px,tmp1,px); 	/* px * 1 / z^2 */
		gfp_mod_mult(tmp1,pz,tmp1); /* 1/z^3 */
		gfp_mod_mult(py,tmp1,py);   /* py * 1 / z^3 */
		gfp_copy(one,pz);

	}

}


/* Computes Q = Q + P where Q is (qx, qy, qz)- Jacobian and P is
* (px, py, 1)(affine).  Elliptic curve points P, Q, and R can all be identical.
* Uses mixed Jacobian-affine coordinates.
* Uses equation (3) from Brown, Hankerson, Lopez, and Menezes.
* Software Implementation of the NIST Elliptic Curves Over Prime Fields. */
static void gfp_point_add_aff_jac(uint32_t *qx,uint32_t *qy,uint32_t *qz,uint32_t *px,uint32_t *py){

	/* 3 temp variables needed for computation of addition and doubling */
	uint32_t tmp1[GFP_ARRAY_LENGHT32];
	uint32_t tmp2[GFP_ARRAY_LENGHT32];
	uint32_t tmp3[GFP_ARRAY_LENGHT32];
	/* If Q is PAI then return P in jacobian coords */
	if(gfp_cmp(qz,zero) == 0){
#ifdef DEBUG_ECC
		DEBUG_MSG(" \n --ECC-- : gfp_point_add_aff_jac : Q was PAI -> returning P\n");
#endif
		gfp_aff_to_jac(px,py,qx,qy,qz);
		return;

	}

	/* If P is PAI then return Q */
	if ((gfp_cmp(px,zero) == 0) && (gfp_cmp(py,zero) == 0)) {
#ifdef DEBUG_ECC
		DEBUG_MSG(" \n --ECC-- : gfp_point_add_aff_jac : P was PAI -> returning Q\n");
#endif
		/*gfp_copy(px,rx);
		gfp_copy(py,ry);
		gfp_copy(pz,rz);*/
		return;
	}


	/* A = px * qz^2, B = py * qz^3 */
	gfp_mod_mult(qz,qz,tmp1);
	gfp_mod_mult(tmp1,qz,tmp2);
	gfp_mod_mult(tmp1,px,tmp1); /* A */
	gfp_mod_mult(tmp2,py,tmp2); /* B */

	/* C = A - qx, D = B - qy */
	gfp_mod_sub(tmp1,qx,tmp1); /* C */
	gfp_mod_sub(tmp2,qy,tmp2); /* D */

	if(gfp_cmp(tmp1,zero) == 0){

		if(gfp_cmp(tmp2,zero) == 0){
#ifdef DEBUG_ECC
			DEBUG_MSG("\n --ECC-- : gfp_point_add_aff_jac: P equals Q , reverting to doubling\n");
#endif
			/* P equals Q so double P */
			gfp_copy(px,qx);
			gfp_copy(py,qy);
			gfp_copy(one,qz);
			gfp_point_double_jac(qx,qy,qz);
			return;
		}
		else {
#ifdef DEBUG_ECC
			DEBUG_MSG("\n --ECC-- : gfp_point_add_aff_jac : Z3 = 0 => return PAI\n");
#endif
			/* Z3 = 0 so result will be PAI */
			/*gfp_copy(px,rx);
			gfp_copy(py,ry); */
			gfp_zeroize(qz);
			return;

		}
	}

	/* qz = qz * C */
	gfp_mod_mult(qz,tmp1,qz);

	/* C = C^2, D = C^3 */
	gfp_mod_mult(tmp1,tmp1,tmp3); /* tmp3 = c^2 */
	gfp_mod_mult(tmp3,tmp1,tmp1); /* tmp1 = c^3 */

	/* C = qx * C^2 */
	gfp_mod_mult(qx,tmp3,tmp3);

	/* qx = D^2 */
	gfp_mod_mult(tmp2,tmp2,qx);

	/* qy = qy*C^3 */
	gfp_mod_mult(tmp1,qy,qy);

	/* tmp1 = D^2 - C^3 */
	gfp_mod_sub(qx,tmp1,tmp1);

	/* qx = 2*qx*C^2 */
	gfp_mod_add(tmp3,tmp3,qx);

	/* qx = C^3 - qX */
	gfp_mod_sub(tmp1,qx,qx);

	/* tmp1 = qX*C^2 - qx */
	gfp_mod_sub(tmp3,qx,tmp1);

	/* tmp2 = D*(qX*C^2  - qx)*/
	gfp_mod_mult(tmp2,tmp1,tmp2);

	/* qy = tmp2 - qy */
	gfp_mod_sub(tmp2,qy,qy);


}

/* Computes P = 2P.  Elliptic curve points P and R can be identical.  Uses
 * Jacobian coordinates. Uses slightly faster doubling algoritm for a == p - 3.
 * Does NOT treat the case when a!=p - 3. See algorithm 3.21 Guide to elliptic curve cryptography */
static void gfp_point_double_jac(uint32_t *px,uint32_t *py,uint32_t *pz){

	uint32_t tmp1[GFP_ARRAY_LENGHT32];
	uint32_t tmp2[GFP_ARRAY_LENGHT32];

	/* If P is PAI then return PAI in jacobian coords */
	if(gfp_cmp(pz,zero) == 0){

		/*gfp_copy(px,rx);
		gfp_copy(py,ry);
		gfp_copy(pz,rz);*/
		return;

	}

	/* tmp1 = pz^2 */
	gfp_mod_mult(pz,pz,tmp1);

	/* pz = pz * py */
	gfp_mod_mult(pz,py,pz);

	/* rz = 2 * py * pz */
	gfp_mod_add(pz,pz,pz);


	/* a = p - 3 stuff
	 * calculates : C = 3(px - pz^2)(px + pz^2)*/
	gfp_mod_sub(px,tmp1,tmp2);
	gfp_mod_add(px,tmp1,tmp1);
	gfp_mod_mult(tmp1,tmp2,tmp2);
	gfp_mod_add(tmp2,tmp2,tmp1);
	gfp_mod_add(tmp1,tmp2,tmp1); /* C */


	/* ry = 2 * py */
	gfp_mod_add(py,py,py);

	/* ry = 4 * py^2 */
	gfp_mod_mult(py,py,py);

	/* tmp2 = 16 * py^4 */
	gfp_mod_mult(py,py,tmp2);

	/* ry = ry * x1 */
	gfp_mod_mult(py,px,py); /* A */
	/* tmp2 = tmp2/2 */
	gfp_divide2(tmp2); 	    /* B */

	/* rx = C^2 - 2*A */    /* D */
	gfp_mod_mult(tmp1,tmp1,px);
	gfp_mod_sub(px,py,px);
	gfp_mod_sub(px,py,px);

	/* ry = (A-D)*C - B */
	gfp_mod_sub(py,px,py);

	gfp_mod_mult(py,tmp1,py);

	gfp_mod_sub(py,tmp2,py);



}


/* Scalar Point Multiplication : Q = kP. Both P and Q are in affine coordinates.
 * If other coordinates are used transformation must take place prior to returning.
 * Uses algorithm 3.27 from Guide to Elliptic Curve Cryptography
 */
void gfp_point_mult(uint32_t *k,uint32_t *px,uint32_t *py,uint32_t *qx){

	int16_t si;
	/* the third coordinate for jacobian representation of Q */
	uint32_t qz[GFP_ARRAY_LENGHT32];
	uint32_t qy[GFP_ARRAY_LENGHT32];


	/* Q <= 0 in jacobian coordinates (x,y,1) */
	gfp_zeroize(qx);
	gfp_zeroize(qy);
	gfp_copy(one,qz);



	for (si = GFP_PRIME_FIELD -1 ; si>=0 ;si--){
		/* Q = 2*Q */
		gfp_point_double_jac(qx,qy,qz);
		if(test_bit(k,si) == 1)
			/* TODO investigate subtle danger if Q=P on manning forum*/
			/* Q = Q+P */
			gfp_point_add_aff_jac(qx,qy,qz,px,py);

	}

	/* convert back to affine */
	gfp_jac_to_aff(qx,qy,qz);

#ifdef DEBUG_ECC
	PRINT_BIG(qx," --ECC-- : Scalar Point Multiplication Result :");
#endif



}




