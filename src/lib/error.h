/*
 * phc -- the open source PHP compiler
 * See doc/license/README.license for licensing information
 * 
 * Error handling 
 */

#ifndef PHC_ERROR_H
#define PHC_ERROR_H

#include <stdarg.h>
#include <stddef.h>

class String;

namespace AST { class Node; }
namespace HIR { class Node; }
namespace MIR { class Node; }

void phc_internal_error (const char* message, ...);
void phc_internal_error (const char* message, va_list args, String* filename, int line);
void phc_internal_error (const char* message, String* filename, int line, ...);
void phc_internal_error (const char* message, AST::Node*, ...);
void phc_internal_error (const char* message, HIR::Node*, ...);
void phc_internal_error (const char* message, MIR::Node*, ...);



void phc_error (const char* message, ...);
void phc_error (const char* message, va_list args, String* filename, int line);
void phc_error (const char* message, String* filename, int line, ...);
void phc_error (const char* message, AST::Node*, ...);
void phc_error (const char* message, HIR::Node*, ...);
void phc_error (const char* message, MIR::Node*, ...);



void phc_warning (const char* message, ...);
void phc_warning (const char* message, va_list args, String* filename, int line); 
void phc_warning (const char* message, String* filename, int line, ...);
void phc_warning (const char* message, AST::Node*, ...);
void phc_warning (const char* message, HIR::Node*, ...);
void phc_warning (const char* message, MIR::Node*, ...);

void __attribute__((noreturn)) phc_unreachable (const char* message = NULL);
void __attribute__((noreturn)) phc_TODO (const char* message = NULL);

#endif // PHC_ERROR_H
