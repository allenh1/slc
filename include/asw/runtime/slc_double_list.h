// Copyright 2024 Hunter L. Allen
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#ifndef ASW__SLC__RUNTIME__SLC_DOUBLE_LIST_H_
#define ASW__SLC__RUNTIME__SLC_DOUBLE_LIST_H_

#include <stdint.h>

struct slc_double_list;

struct slc_double_list * slc_int_list_create();
int8_t slc_int_list_destroy(struct slc_double_list *);
int8_t slc_int_list_init(struct slc_double_list *);
int8_t slc_int_list_fini(struct slc_double_list *);
int8_t slc_int_list_set_head(struct slc_double_list *, double);
int8_t slc_int_list_set_tail(struct slc_double_list *, struct slc_double_list *);

/* unary ops */
double * slc_int_list_car(struct slc_double_list *);
struct slc_double_list * slc_double_list_cdr(struct slc_double_list *);


/* binary ops */
struct slc_double_list * slc_double_list_cons(double head, struct slc_double_list * tail);
struct slc_double_list * slc_double_list_append(struct slc_double_list *, double);

/* list ops */
double slc_double_list_add(struct slc_double_list *);
double slc_double_list_subtract(struct slc_double_list *);
double slc_double_list_multiply(struct slc_double_list *);
double slc_double_list_divide(struct slc_double_list *);

/* print */
int64_t print_double(double);

#endif  /* ASW__SLC__RUNTIME__SLC_DOUBLE_LIST_H_ */
