(set-logic QF_NRA)
(declare-const r Real)
(assert (< r 0))
(assert (= 1.0 (/ 2 r r)))
(check-sat)
(get-value (r))

