/* Webcamod, webcam capture plasmoid.
 * Copyright (C) 2011-2012  Gonzalo Exequiel Pedone
 *
 * Webcamod is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * Webcamod is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Webcamod. If not, see <http://www.gnu.org/licenses/>.
 *
 * Email     : hipersayan DOT x AT gmail DOT com
 * Web-Site 1: http://github.com/hipersayanX/Webcamoid
 * Web-Site 2: http://kde-apps.org/content/show.php/Webcamoid?content=144796
 */

%{
#include <QtScript>

#include "parser_auto.h"

int yycolumn = 1;

#define YY_USER_ACTION yylloc.first_line = yylineno; \
                       yylloc.first_column = yycolumn; \
                       yylloc.last_line = yylineno; \
                       yylloc.last_column = yycolumn + yyleng - 1; \
                       yycolumn += yyleng;
%}

%option yylineno
%s PARSEBITS

%%

\( return TOK_LEFTPAREN;

\) {
    if (YY_START == PARSEBITS)
        BEGIN(INITIAL);

    return TOK_RIGHTPAREN;
}

\{ return TOK_LEFTCURLYBRACKET;
\} return TOK_RIGHTCURLYBRACKET;
\[ return TOK_LEFTBRACKET;
\] return TOK_RIGHTBRACKET;
\< return TOK_LEFTANGLEBRACKET;
\> return TOK_RIGHTANGLEBRACKET;
\/ return TOK_SLASH;
!  return TOK_EXCL;
\. return TOK_DOT;
,  return TOK_COMMA;
:  return TOK_COLON;
=  return TOK_EQUAL;

<PARSEBITS>(1|0)+([ \t\r\n]*(1|0)+)* {
    yylval.QVariant_t = new QVariant();
    QString bitsString(yytext);
    QBitArray bits;

    bitsString.replace(QRegExp("\\s+"), "");

    if (bitsString.length() > 0)
    {
        bits.resize(bitsString.length());

        for (int i = 0; i < bitsString.length(); i++)
            bits.setBit(i, (bitsString[i] == '0')? false: true);
    }

    *yylval.QVariant_t = bits;

    return TOK_BITVALUES;
}

-?[0-9]+ {
    yylval.QVariant_t = new QVariant();
    *yylval.QVariant_t = QString(yytext).toInt();

    return TOK_INTIGER;
}

-?([0-9]+\.[0-9]+|\.[0-9]+|[0-9]+\.) {
    yylval.QVariant_t = new QVariant();
    *yylval.QVariant_t = QString(yytext).toFloat();

    return TOK_FLOAT;
}

true|false {
    yylval.QVariant_t = new QVariant();

    if (QString(yytext) == "true")
        *yylval.QVariant_t = true;
    else
        *yylval.QVariant_t = false;

    return TOK_BOOLEAN;
}

size     return TOK_SIZE;
sizeF    return TOK_SIZEF;
point    return TOK_POINT;
pointF   return TOK_POINTF;
rect     return TOK_RECT;
rectF    return TOK_RECTF;
line     return TOK_LINE;
lineF    return TOK_LINEF;
date     return TOK_DATE;
time     return TOK_TIME;
dateTime return TOK_DATETIME;
color    return TOK_COLOR;
b        return TOK_BYTES;
url      return TOK_URL;

bits {
    BEGIN(PARSEBITS);

    return TOK_BITS;
}

\"([^"\\]*|\\.)*\"|'([^'\\]*|\\.)*' {
    yylval.QVariant_t = new QVariant();
    QScriptEngine scriptEngine;
    *yylval.QVariant_t = scriptEngine.evaluate(QString(yytext)).toString();

    return TOK_STRING;
}

IN\.  return TOK_REFIN;
OUT\. return TOK_REFOUT;

[a-zA-Z_][0-9a-zA-Z_]* {
    yylval.QString_t = new QString(yytext);

    return TOK_IDENTIFIER;
}

. // Unrecognized token.

%%
