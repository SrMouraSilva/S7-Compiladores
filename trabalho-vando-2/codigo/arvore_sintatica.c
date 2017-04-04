#include <stdio.h>
#include <stdlib.h>

#include "arvore_sintatica.h"
#include "common.h"
#include "mensagem.h"

/***********************************
 * Operadores
 ***********************************/
int is_operacao_aritmetica(OperacaoExpressao operacao) {
   return operacao == ADICAO
       || operacao == SUBTRACAO
       || operacao == MULTIPLICACAO
       || operacao == DIVISAO;
}

int is_operacao_logica(OperacaoExpressao operacao) {
   return operacao == OR
       || operacao == AND
       || operacao == NOT;
}

int is_operacao_relacional(OperacaoExpressao operacao) {
   return operacao == IGUAL
       || operacao == DIFERENTE
       || operacao == MENOR_QUE
       || operacao == MENOR_IGUAL_QUE
       || operacao == MAIOR_QUE
       || operacao == MAIOR_IGUAL_QUE;
}

/***********************************
 * Nós
 ***********************************/
const char * OperacaoExpressaoDescricao[] = {
  "constante", "variavel",

  "+", "-", "*", "/",

  ">", ">=", "<", "<=",
  "==", "!=",

  "||", "&&", "!"
};

/***********************************
 * Nós
 ***********************************/
/**
 * @return NoAST default, com todos os valores NULL
 */
NoAST * no_new(void * no_filho, NoTipoAST tipo) {
  NoAST * no = calloc(1, sizeof(NoAST));

  no->no = no_filho;
  no->tipo = tipo;

  return no;
}

NoAST * no_new_raiz(ListaEncadeada * declaracoes, ListaEncadeada * comandos) {
  NoRaizAST * no = calloc(1, sizeof(NoRaizAST));

  no->declaracoes = declaracoes;
  no->comandos = comandos;

  return no_new(no, AST_TIPO_RAIZ);
}

NoAST * no_new_repeticao_for(Simbolo * variavel, NoAST * expressao_inicio, NoAST * expressao_fim, ListaEncadeada * comandos) {
  NoRepeticaoForAST * no = calloc(1, sizeof(NoRepeticaoForAST));

  no->variavel = variavel;
  no->expressao_inicio = expressao_inicio;
  no->expressao_fim = expressao_fim;
  no->comandos = comandos;

  return no_new(no, AST_TIPO_REPETICAO_FOR);
}

NoAST * no_new_repeticao_while(NoAST * expressao, ListaEncadeada * comandos) {
  NoRepeticaoWhileAST * no = calloc(1, sizeof(NoRepeticaoWhileAST));

  no->expressao = expressao;
  no->comandos = comandos;

  return no_new(no, AST_TIPO_REPETICAO_WHILE);
}

NoAST * no_new_atribuicao(Simbolo * variavel, NoAST * expressao) {
  NoAtribuicaoAST * no = calloc(1, sizeof(NoAtribuicaoAST));

  no->variavel = variavel;
  no->expressao = expressao;

  return no_new(no, AST_TIPO_ATRIBUICAO);
}

NoAST * no_new_print(NoAST * expressao) {
  NoPrintAST * no = calloc(1, sizeof(NoPrintAST));

  no->expressao = expressao;

  return no_new(no, AST_TIPO_PRINT);
}

NoAST * no_new_input(Simbolo * variavel) {
  NoInputAST * no = calloc(1, sizeof(NoInputAST));

  no->variavel = variavel;

  return no_new(no, AST_TIPO_INPUT);
}

NoAST * no_new_if_else(NoAST * expressao, ListaEncadeada * comandos_if, ListaEncadeada * comandos_else) {
  NoIfElseAST * no = calloc(1, sizeof(NoIfElseAST));

  no->expressao = expressao;
  no->comandos_if = comandos_if;
  no->comandos_else = comandos_else;

  return no_new(no, AST_TIPO_IF_ELSE);
}

NoAST * no_new_constante(void * valor, SimboloTipo tipo) {
  NoConstanteAST * no = calloc(1, sizeof(NoConstanteAST));

  no->valor = valor;
  no->tipo = tipo;

  return no_new(no, AST_TIPO_CONSTANTE);
}

/**********************************
 * Expressão
 * ********************************/
static void erro_operacao_tipo(char * posicao, NoAST * no, char * operacao, SimboloTipo tipo_operacao) {
  char * mensagem = mensagem_preparar(
    "Símbolo à %s da operação %s‘%s’%s é do tipo %s‘%s’%s, mas a operação aceita somente %s‘%s’%s\n",
    posicao,
    "\033[1;97m", operacao, "\033[0m",
    "\033[1;97m", SimboloTipoDescricao[no->tipo], "\033[0m",
    "\033[1;97m", SimboloTipoDescricao[tipo_operacao], "\033[0m"
  );

  mensagem_erro(yy_nome_arquivo, yylineno, 0, mensagem);
  free(mensagem);
}

static void verificar_tipos_elementos(NoExpressaoAST * no);
static void verificar_operacao_inteiros(NoExpressaoAST * no);
static void verificar_operacao_string_e_inteiros(NoExpressaoAST * no);

static SimboloTipo tipo(NoAST * no);

NoAST * no_new_expressao(NoAST * no_esquerda, OperacaoExpressao operacao, NoAST * no_direita) {
  NoExpressaoAST * no = calloc(1, sizeof(NoExpressaoAST));

  no->esquerda = no_esquerda;
  no->operacao = operacao;
  no->direita = no_direita;

  verificar_tipos_elementos(no);

  return no_new(no, AST_TIPO_EXPRESSAO);
}

static void verificar_tipos_elementos(NoExpressaoAST * no) {
  verificar_operacao_inteiros(no);
  verificar_operacao_string_e_inteiros(no);
}

static void verificar_operacao_inteiros(NoExpressaoAST * no) {
  if (no->operacao != SUBTRACAO
   && no->operacao != MULTIPLICACAO
   && no->operacao != DIVISAO)
    return;

  char * operacao = (char *) OperacaoExpressaoDescricao[no->operacao];

  if (no->esquerda != NULL && tipo(no->esquerda) != SIMBOLO_TIPO_INTEIRO)
    erro_operacao_tipo("esquerda", no->esquerda, operacao, SIMBOLO_TIPO_INTEIRO);
  if (no->direita != NULL && tipo(no->direita) != SIMBOLO_TIPO_INTEIRO)
    erro_operacao_tipo("direita", no->direita, operacao, SIMBOLO_TIPO_INTEIRO);
}

static void verificar_operacao_string_e_inteiros(NoExpressaoAST * no) {
  if (no->operacao != ADICAO)
    return;

  SimboloTipo tipo_esquerda = tipo(no->esquerda);
  SimboloTipo tipo_direita = tipo(no->direita);

  if (tipo_esquerda == SIMBOLO_TIPO_INTEIRO && tipo_direita == SIMBOLO_TIPO_INTEIRO
   || tipo_esquerda == SIMBOLO_TIPO_STRING  && tipo_direita == SIMBOLO_TIPO_STRING)
   return;

  char * mensagem = mensagem_preparar(
    "Símbolos à esquerda e à direita da operação %s‘%s’%s devem ser ambos do tipo %s‘%s’%s ou %s‘%s’%s\n",
    "\033[1;97m", "adição (+)", "\033[0m",
    "\033[1;97m", SimboloTipoDescricao[SIMBOLO_TIPO_INTEIRO], "\033[0m",
    "\033[1;97m", SimboloTipoDescricao[SIMBOLO_TIPO_STRING], "\033[0m"
  );

  mensagem_erro(yy_nome_arquivo, yylineno, 0, mensagem);
  free(mensagem);
}

static SimboloTipo tipo(NoAST * no) {
  if (no->tipo == AST_TIPO_EXPRESSAO) {
    NoExpressaoAST * expressao = (NoExpressaoAST *) no->no;
    return expressao->tipo;

  } else if (no->tipo == AST_TIPO_CONSTANTE) {
    NoConstanteAST * constante = (NoConstanteAST *) no->no;
    return constante->tipo;

  } else {
    NoVariavelAST * no_variavel = (NoVariavelAST *) no->no;
    return no_variavel->valor->tipo;
  }
}
