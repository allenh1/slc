;; declare external print_double function
(extern int print_double (d: float))

(defun foo (x: float, y: float)
  (* (+ x y) 4.0))

(defun main (argc: int)
  (print_double (foo argc 0)))