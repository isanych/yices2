(set-logic QF_AX)
(declare-sort Element 0)
(declare-sort Index 0)
(declare-fun i () Index)
(declare-fun j () Index)
(declare-fun a () (Array Index Element))
(declare-fun b () (Array Index Element))
(declare-fun c () (Array Index Element))
(declare-fun v () Element)
(declare-fun w () Element)
(assert (= b (store a i v)))
(assert (= c (store a j w)))
(assert (not (= i j)))
(assert (not (= (store b j w) (store c i v))))
(check-sat)
