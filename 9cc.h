#ifndef __INCLUDE_9CC_H__
#define __INCLUDE_9CC_H__

/*--------------------------------------------------------------------*/
/*! @brief  トークンの種類
 */
typedef enum {
    TK_RESERVED = 0,    //!< 記号
    TK_IDENT,           //!< 識別子
    TK_NUM,             //!< 整数トークン
    TK_EOF,             //!< 入力の終わりを表すトークン
} TokenKind_t;

/*--------------------------------------------------------------------*/
/*! @brief  トークン型
 */
typedef struct Token_ {
    TokenKind_t kind;       //!< トークンの型
    struct Token_ *next;    //!< 次の入力トークン
    int val;                //!< kindがTK_NUMの場合、その数値
    char *str;              //!< トークン文字列
    int len;                //!< トークンの長さ
} Token_t;

/*--------------------------------------------------------------------*/
/*! @brief  抽象構文木のノードの種類
 */
typedef enum {
    ND_ADD = 0,     //!< +
    ND_SUB,         //!< -
    ND_MUL,         //!< *
    ND_DIV,         //!< /
    ND_GRT,         //!< > ※左右ノードを入れ替えてND_LSSとして処理するため使用しない
    ND_LSS,         //!< <
    ND_EQU,         //!< ==
    ND_NOT_EQU,     //!< !=
    ND_GRT_EQU,     //!< >= ※左右ノードを入れ替えてND_LSS_EQUとして処理するため使用しない
    ND_LSS_EQU,     //!< <=
    ND_ASSIGN,      //!< =
    ND_LVAR,        //!< left value
    ND_NUM,         //!< 整数 
} NodeKind_t;

/*--------------------------------------------------------------------*/
/*! @brief  抽象構文木のノードの型
 */
typedef struct Node_ {
    NodeKind_t kind;    //!< ノードの型
    struct Node_ *lhs;  //!< 左辺
    struct Node_ *rhs;  //!< 右辺
    int val;            //!< kindがND_NUMの場合のみ使用する
    int offset;         //!< kindがND_LVARの場合のみ使用する
} Node_t;

/*--------------------------------------------------------------------*/
/*! @brief  ローカル変数の型
 */
typedef struct LVar_ {
    struct LVar_ *next; //!< 次の変数 or NULL
    char *name;         //!< 変数名
    int len;            //!< 変数名の長さ
    int offset;         //!< RBPからのオフセット
} LVar_t;

/*--------------------------------------------------------------------*/
/*! @brief  エラー報告関数
 */
void error(char *fmt, ...);

/*--------------------------------------------------------------------*/
/*! @brief  入力文字列pをトークナイズして返す
 */
void tokenize(char *p);

/*--------------------------------------------------------------------*/
/*! @brief  構文解析を実行
 */
void program(void);

/*--------------------------------------------------------------------*/
/*! @brief  アセンブラコードを生成する
 */
void gen(Node_t *node);

/*--------------------------------------------------------------------*/
/*! @brief  構文解析結果を取得
 */
void get_code(Node_t *p[100]);

/*--------------------------------------------------------------------*/
/*! @brief  初期化
 */
void parser_init(char *p);

#endif  /* __INCLUDE_9cc_H__ */