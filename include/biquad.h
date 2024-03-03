#ifndef _BIQUAD_H
#define _BIQUAD_H
#include "node.h"

//! Set all filter coefficients.
// void setCoefficients( StkFloat b0, StkFloat b1, StkFloat b2, StkFloat a1,
// StkFloat a2, bool clearState = false );
//
// //! Set the b[0] coefficient value.
// void setB0( StkFloat b0 ) { b_[0] = b0; };
//
// //! Set the b[1] coefficient value.
// void setB1( StkFloat b1 ) { b_[1] = b1; };
//
// //! Set the b[2] coefficient value.
// void setB2( StkFloat b2 ) { b_[2] = b2; };
//
// //! Set the a[1] coefficient value.
// void setA1( StkFloat a1 ) { a_[1] = a1; };
//
// //! Set the a[2] coefficient value.
// void setA2( StkFloat a2 ) { a_[2] = a2; };
typedef struct {
  double b[3];
  double a[3];
  double inputs_[3];
  double outputs_[3];
  double K_;
  double kSqr_;
  double denom_;
  double gain;
} biquad_state;
Node *biquad_node(Node *in);

Node *biquad_lp_node(double freq, double res, Node *in);
#endif
