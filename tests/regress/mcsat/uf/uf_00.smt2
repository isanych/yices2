(set-logic QF_UF)
(declare-sort s 0)
(declare-fun x () s)
(declare-fun y () s)
(assert (= x y))
(check-sat)
(exit)
