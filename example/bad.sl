(let x (* 3 4))
(let y (+ 1 2))
;; (defun y (+ 1))  ;; redefinition error
;; (let t (+ z 3))  ;; undefined reference error

(defun foo [bar]
  (+ bar))

(defun main [args]
  (print (foo args)))
