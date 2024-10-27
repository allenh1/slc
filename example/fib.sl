(defun fib (fn: list<int>)
  (let n (car fn))
  (let fibs (if (= nil (cdr fn)) '(1 0) (cdr fn)))
  (if (> n 1)
    (fib (cons (- n 1) (cons (+ (extract_two fibs)) fibs)))
    fibs))

(defun extract_two (fn: list<int>)
  (cons (car fn) (car (cdr fn))))
