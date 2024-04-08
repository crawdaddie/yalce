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
  double b0, b1, b2; // Feedforward coefficients
  double a1, a2;     // Feedback coefficients
  double x1, x2;     // Input delay elements (x[n-1] and x[n-2])
  double y1, y2;     // Output delay elements (y[n-1] and y[n-2])
} biquad_state;

void init_biquad_filter_state(biquad_state *filter, double b0, double b1,
                              double b2, double a1, double a2);
Node *biquad_node(Node *in);

Node *biquad_lp_node(double freq, double res, Node *in);
Node *biquad_lp_dyn_node(double freq, double res, Node *in);

Node *biquad_hp_dyn_node(double freq, double res, Node *in);

Node *biquad_hp_node(double freq, double res, Node *in);

Node *biquad_bp_node(double freq, double res, Node *in);
Node *biquad_bp_dyn_node(double freq, double res, Node *in);

Node *butterworth_hp_dyn_node(double freq, Node *in);

#endif
