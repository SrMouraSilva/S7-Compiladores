%{
#include <stdio.h>
#include <stdarg.h>

#include "common.h"
#include "logica.h"
#include "mensagem.h"

#include "regras.tab.h"
#include "conversor.h"

extern int yylex();
extern void yyerror(char *s, ...);

#define YY_USER_ACTION update_localization();
%}
%locations

%code requires {
    #include "tabela.h"
    #include "arvore_sintatica.h"
}

// yylval
%union {
 // Identificador de variável na tabela de símbolos
 Simbolo * simbolo;

 // Declarar tipo de uma variável
 SimboloTipo tipo;

 // Algo que não foi totalmente processado
 NoAST * no;
}


// http://www.gnu.org/software/bison/manual/html_node/Token-Decl.html#Token-Decl
// Tipos
%token t_int
%token t_bool
%token t_string
%token <simbolo> t_variavel

// Palavras reservadas
%token t_var // Bloco de definição de variáveis
%token t_begin // Bloco de código
%token t_end // Fim do código

// Análise léxica detecta e trata algum erro
%token t_noop
// Fim do arquivo
%token t_eof

// Tokens da gramática: Elementos não terminais
%type<tipo> tipo
%type<no> variavel_declaracao
%type<no> lista_variaveis variavel

/************************************************************/
%%
/************************************************************/

programa: t_var variavel_declaracao t_begin t_end fim_codigo {
//programa: t_var lista_declaracoes t_begin lista_comandos t_end fim_codigo {
  //no_new_raiz($2, $4);
  NoAST * no = no_new_raiz($2, NULL);
  imprimir_codigo(no);
  //no_free(no);
  exit(0);
}
;


/******************************
 * Declarações
 ******************************/
lista_declaracoes: lista_declaracoes ';' variavel_declaracao
                 | variavel_declaracao {printf("passou por aqui\n");}
;

variavel_declaracao: tipo ':' lista_variaveis {
  logica_declarar_lista_variaveis($3, $1);
  $$ = $3;
}
;

tipo: t_int    {$$ = SIMBOLO_TIPO_INTEIRO;}
    | t_bool   {$$ = SIMBOLO_TIPO_BOOLEANO;}
    | t_string {$$ = SIMBOLO_TIPO_STRING;}
;

lista_variaveis: lista_variaveis ',' variavel { $$ = no_vincular_elementos_lista_encadeada($1, $3); }
               | variavel { $$ = $1; }
;

variavel: t_variavel { $$ = no_new_elemento_lista_encadeada($1); }
;

/******************************
 * Comandos
 ******************************/

/******************************
* Fim de arquivo
******************************/

fim_codigo: t_eof { /*ast_imprimir($$);*/ }
;

%%

int main() {
  printf("Vando 1.0.0 (64-bit)\n");
  printf("[GCC %d.%d.%d] on linux\n", __GNUC__, __GNUC_MINOR__, __GNUC_PATCHLEVEL__);

  return yyparse();
}