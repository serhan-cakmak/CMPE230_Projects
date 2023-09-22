; serhan cakmak
; 2020400225
; compiling: yes
; complete: yes
#lang racket

(provide (all-defined-out))
(require racket/trace)
; read and parse the input file
(define parse (lambda (input-file)
        (letrec (
            [input-port (open-input-file input-file)]
            [read-and-combine (lambda ()
                (let ([line (read input-port)])
                    (if (eof-object? line)
                        '()
                        (append `(,line) (read-and-combine))
                    )
                )
            )]
            )
            (read-and-combine)
        )
    )
)
(define create-hash (lambda (vars values)
        (letrec (
            [create-hash-iter (lambda (vars values hash)
                (if (null? vars)
                    hash
                    (create-hash-iter (cdr vars) (cdr values) (hash-set hash (car vars) (car values)))
                )
            )]
            )
            (create-hash-iter vars values (hash))
        )
    )
)

(define add-to-hash (lambda (old-hash new-hash)
        (foldl (lambda (key hash) (hash-set hash key (hash-ref new-hash key)))
            old-hash
            (hash-keys new-hash)
        )
    )
)

(define eval-program (lambda (program-str)
        (get (eval-exprs (parse program-str) empty-state) '-r)
    )
)

; solution starts here


; 1. empty-state (5 points)
(define empty-state (hash) )  ;calling hash without arguments creates an empty hash table
; 2. get (5 points)
(define (get state var ) (if (hash-has-key? state var )    (hash-ref state var)  (eval var)))  ;check if the variable is in the hash if not, evaluate it.
; 3. put (5 points)
(define (put state var val) (hash-set state var val) ) ;set the value of the variable in the hash 
; 4. := (15 points)
(define (:= var val-expr state)
           ;again check the type (var?) vs. and get the value. (val-expr would be enough. )
  (put (put state var (get (eval-expr val-expr state) '-r )) '-r  (get (eval-expr val-expr state) '-r ))  ;get the state after evaluating the expression and set the value of the variable in the hash.
  )
; 5. if: (15 points)
(define (if: test-expr then-exprs else-exprs state)
  ;get the truth value from eval expr and evaluate the then or else expression accordingly.
  (if (get (eval-expr test-expr state) '-r)  (eval-exprs then-exprs (eval-expr test-expr state)) (eval-exprs else-exprs (eval-expr test-expr state)))      
               
  )
; 6. while: (15 points)
(define (while: test-expr body-exprs state)
  ;(display (get (eval-expr test-expr state) '-r))
  ;if true evaluate call whle expr again with evalueated values. 
  (if  (get (eval-expr test-expr state) '-r)  (while: test-expr body-exprs ( eval-exprs body-exprs state) ) (put state '-r #f) )  
  
  )
; 7. func (15 points)
;zip the params and values and put them in the hash. and return the function not the value.
(define (func params body-exprs state)    (put state '-r (lambda values (get (eval-exprs body-exprs (foldl (lambda (x y z) (put z x y)) state params values )) '-r)))    )


;function that replaces variables with their values and also evalueates list expressions.
(define (var-replacer list state)
  (map (lambda (x)
         (if
           (list? x)
           (get (eval-expr x state) '-r)  
           (if (hash-has-key? state x)  (get state x) (eval x)) 

           ))
          list )
  
  )
(define (map-eval list state)
  ;(display list)
  ;(display (var-replacer (cdr list) state))
  ;apply the function to the list of values or lists(var-replacer will take care of it by calling eval-expr).
  (apply (get ( eval-expr (car list) state) '-r) (var-replacer (cdr list) state))
  
  )
; 8. eval-expr (20 points)
(define (eval-expr expr state)
   (cond
     ((list? expr)
      (cond
        
      ;if the first element one of the following functions, apply the function to the rest of the list.
      [(member (car expr) '(:= if: while: func))
        (apply (eval (car expr)) (cdr (append expr (list state))))
      ]
      ;if the first element is lambda, create a function calling func function.
      [(eq? (car expr) 'lambda)                                   ;you havent test this. (func params body-exprs state) | (lambda (x) (* x x))
        (func (car (cdr expr)) (cddr expr)   state)]

      [ (procedure? (car expr) )
       (put state '-r (map-eval expr state) ) ]
     
       [ (symbol? (car expr) )
       ;  (apply (eval (car expr)) (cdr))
       (put state '-r (map-eval expr state) )
         ]
       
        

       ;[else (put state '-r (if (integer? expr) expr (get state expr)))]
       
       
       ))


      ; if the expresion is literal or integer.
     [else (put state '-r (if (integer? expr) expr (get state expr)))]
      )

)
  


; 9 eval-exprs (5 points)
(define (eval-exprs exprs state)
  
  (foldl eval-expr state exprs )

  )