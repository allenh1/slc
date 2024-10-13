(defun reverse [fn]
  (+ 1 2)
  (let head (car fn))
  (if (= nil tail)
    '(head)
    (cons (reverse tail) '(head))))
