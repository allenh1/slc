(extern int slc_puts(s: string))
(extern int slc_read_int)
(extern bool print_slc_int_list(d: list<int>))

(defun range_impl(a: int, b: int, acc: list<int>)
  (if (= b a)
    acc
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
  (for idx in (range 0 n)
    (let tmp f2)
    (set f2 (+ f1 f2))
    (set f1 f2)
  )
  f2)

(defun main
  (slc_puts "How many fns do you want?")
  (print_slc_int_list (range 1 (slc_read_int))))
