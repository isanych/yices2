(set-info :source |fuzzsmt|)
(set-info :smt-lib-version 2.0)
(set-info :category "random")
(set-info :status unknown)
(set-logic QF_BV)
(declare-fun v0 () (_ BitVec 6))
(declare-fun v1 () (_ BitVec 5))
(declare-fun v2 () (_ BitVec 7))
(declare-fun v3 () (_ BitVec 6))
(assert (let ((e4(_ bv12 4)))
(let ((e5 ((_ sign_extend 2) e4)))
(let ((e6 ((_ zero_extend 0) v2)))
(let ((e7 (ite (= (_ bv1 1) ((_ extract 4 4) v1)) v2 ((_ zero_extend 2) v1))))
(let ((e8 (ite (= e5 v0) (_ bv1 1) (_ bv0 1))))
(let ((e9 ((_ extract 0 0) v2)))
(let ((e10 (ite (= (_ bv1 1) ((_ extract 3 3) v1)) v3 ((_ sign_extend 5) e8))))
(let ((e11 (= v2 ((_ sign_extend 6) e9))))
(let ((e12 (= e5 ((_ zero_extend 2) e4))))
(let ((e13 (= e5 ((_ zero_extend 5) e8))))
(let ((e14 (distinct v0 ((_ zero_extend 5) e8))))
(let ((e15 (= ((_ sign_extend 4) e9) v1)))
(let ((e16 (= v2 ((_ zero_extend 2) v1))))
(let ((e17 (= e10 v3)))
(let ((e18 (distinct ((_ zero_extend 5) e8) v3)))
(let ((e19 (distinct ((_ sign_extend 1) e10) e6)))
(let ((e20 (= ((_ sign_extend 6) e8) e6)))
(let ((e21 (distinct ((_ sign_extend 6) e8) v2)))
(let ((e22 (= ((_ zero_extend 1) e5) v2)))
(let ((e23 (distinct e6 ((_ zero_extend 6) e9))))
(let ((e24 (distinct ((_ zero_extend 1) v0) v2)))
(let ((e25 (distinct ((_ sign_extend 5) e8) v3)))
(let ((e26 (= v3 v0)))
(let ((e27 (distinct ((_ zero_extend 3) e4) v2)))
(let ((e28 (distinct ((_ zero_extend 6) e9) e7)))
(let ((e29 (=> e17 e25)))
(let ((e30 (xor e29 e29)))
(let ((e31 (not e28)))
(let ((e32 (not e19)))
(let ((e33 (= e12 e14)))
(let ((e34 (xor e26 e11)))
(let ((e35 (ite e30 e31 e21)))
(let ((e36 (not e23)))
(let ((e37 (ite e27 e27 e24)))
(let ((e38 (ite e16 e32 e13)))
(let ((e39 (and e18 e22)))
(let ((e40 (xor e38 e39)))
(let ((e41 (not e35)))
(let ((e42 (= e20 e37)))
(let ((e43 (= e34 e42)))
(let ((e44 (and e40 e40)))
(let ((e45 (= e43 e44)))
(let ((e46 (and e45 e36)))
(let ((e47 (ite e33 e46 e33)))
(let ((e48 (=> e15 e47)))
(let ((e49 (not e48)))
(let ((e50 (and e41 e41)))
(let ((e51 (xor e49 e50)))
e51
)))))))))))))))))))))))))))))))))))))))))))))))))

(check-sat)
