;; (let x (+ "foo: " (* 2.0 3 4)))
(+ (* 2 3) (+ 1 1))
(car '(1 3))

;; (let x (+ 1 3))
;; (let y (- 3 1))
;; (let z (* x y))

(defun foo (x: int, y: int)
  (> (+ x y) 4))

(defun main (argc: int)
  (foo argc 2))
