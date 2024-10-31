(let x (* 3 4))        ;; x (int) <- 3 * 4 = 12
(let y (+ x 2))        ;; y (int) <- x + 2 = 12 + 2 = 14
;; (let l (cons x '(y)))  ;; l (list<int>) <- '(x y) = '(12 14)
;; (let m (cdr l))        ;; l (list<int>) <- '(y) = '(14)

(defun sum_list (ll: list<int>)               ;; '(3 -3 2)
  (let ret
    (if ll
      (if (not (< 0 (car ll)))
         (+ (car ll) (sum_list (cdr ll)))
         (- (car ll) (sum_list (cdr ll))))
      0))
  ret)

(defun foo (l: list<int>, m: list<int>)
  (let x (sum_list l))
  (let y (sum_list m))
  (* x y))

;;(defun main (argc: int, args: list<string>)
  ;; (print (car m) x (cdr l)))
  ;;  (print (car m) x (car (cdr l))))
