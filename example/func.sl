(defun foo [bar] (+ (cons 1 bar)))

(let x '(3 2 1))
(foo '(3 2))

(defun baz
  (foo '(3 2)))

(print (car (baz nil)))
