(extern int slc_puts(s: string))
(extern int slc_read_int(s: string))
(extern bool print_slc_int_list (d: list<int>))
(extern int slc_int_list_add(d: list<int>))

(defun extract_two (l: list<int>)
 '((car l) (car (cdr l))))

(defun fns (n: int, last: int, fibs: list<int>)
  (if (= last n) fibs
    (fns (+ n 1) last (cons (slc_int_list_add (extract_two fibs)) fibs))))

(defun fib (n: int) (if (= n 2) '(1 1) (if (= n 1) '(1) (fns 2 n '(1 1)))))

(defun main
  (slc_puts "How many fn do you want?")
  (print_slc_int_list (fib (slc_read_int ""))))
