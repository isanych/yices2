(set-logic QF_LRA)
(declare-const a Real)
(declare-const b Real)
(assert (< a b))
(check-sat)
(get-value ((/ a b) (/ b 0)))
