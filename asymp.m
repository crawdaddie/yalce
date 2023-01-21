% asymp.m
%
% A Matlab script for exploring exponential amplitude envelopes.
%
% by Gary Scavone
% McGill University, 2004.

% We use an algorithm of the form:
%
%   y[n] = a y[n-1] + (1-a) target,
%
% where a = exp(-T/tau), T is the sample period and tau is a time constant.
% We assume the user specifies the time constant and target value.
%
% Note that for larger tau values, the curve may not reach the intended
% target within the specified time duration.

fs = 44100;
T = 1/fs;
tau = 0.3;

y = [4 0 ];    % y1 and y2 values
t = [-1 3];    % t1 and t2 values
tinc = T;
a = exp(-T/tau);

% Do iteratively.

tvals = t(1):tinc:t(2);
yvals = zeros( size(tvals) );
yvals(1) = y(1);
tn = t(1);

for n = 2:length(tvals),
  yvals(n) = a*yvals(n-1) + (1-a)*y(2);
end

plot( tvals, yvals )
xlabel('Time');
ylabel('Amplitude');


