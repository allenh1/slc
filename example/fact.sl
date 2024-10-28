;; declare external print_int function
(extern int print_int (d: int))

(defun fact (n: int)
  (if (> n 0)
    (* n (fact (- n 1)))
    1))

(defun main (argc: int)
  (print_int (fact (- argc 1))))
