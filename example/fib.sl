(defun fib [fn]
  (let n (car fn))
  (let fibs (if (= nil (cdr fn)) '(1 0) (cdr fn)))
  (if (> n 1)
    (fib (cons (- n 1) (cons (+ (extract_two fibs)) fibs)))
    fibs))

(defun extract_two [fn]
  (cons (car fn) (car (cdr fn))))
