(defun foo (bar: list<int>) (+ (cons 1 bar)))

(let x '(3 2 1))  ;; declare a list<int> x
(foo '(3 2))      ;; call the function foo on the list '(3 2)

(defun baz (foo '(3 2)))

(print (car (baz nil)))
