(extern int print_int (d: int))

(defun fib (n: int)
  (if (= n 0)
    1
    (if (= n 1)
      1
      (+ (fib (- n 1)) (fib (- n 2))))))

(defun main (n: int)
  (print_int (fib (- n 1))))
