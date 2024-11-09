(extern int print_int (d: int))
(extern int slc_puts(s: string))
(extern int slc_str_to_int(s: string))
(extern int slc_read_int)

(defun fib (n: int)
  (if (< n 2)
    1
    (+ (fib (- n 1)) (fib (- n 2)))))

(defun main
  (slc_puts "which fn do you want?")
  (print_int (fib (slc_read_int))))
