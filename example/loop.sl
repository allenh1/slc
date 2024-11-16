(extern int print_int (d: int))
(extern int slc_puts(s: string))
(extern int slc_read_int)
(extern bool print_slc_int_list(d: list<int>))

(defun range_impl(a: int, b: int, acc: list<int>)
  (if (> a b)
    acc  ;; if a > b, then return.
    (range_impl (+ 1 a) b (cons a acc))))

(defun reverse (a: list<int>)
  (let reverse_impl
    (lambda (a: list<int>, b: list<int>)
      (if (= nil (cdr a))
        (cons (car a) b)
        (reverse_impl (cdr a) (cons (car a) b)))))
  (if (= nil (cdr a)) a (reverse_impl (cdr a) '((car a)))))

(defun range (a: int, b: int)
  (reverse (range_impl (+ a 1) b '(a))))

(defun fn (n: int)
  (let f1 0)
  (let f2 1)
  (if (<= n 1)
    1
    (loop for idx in (range 1 n) do
      (let tmp (+ f1 f2))
      (set f1 f2)
      (set f2 tmp)
    )
  ))

(defun squares (n: int)
  (loop for n in (range 1 (+ n 1)) collect (* n n)))

(defun main
  (slc_puts "Which fn do you want?")
  (let x (slc_read_int))
  (print_int (fn x))
  (slc_puts "squares of n numbers:")
  (print_slc_int_list (squares x))
)
