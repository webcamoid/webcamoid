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
#include "parser_auto.h"

int yycolumn = 1;

#define YY_USER_ACTION yylloc.first_line = yylineno; \
                       yylloc.first_column = yycolumn; \
                       yylloc.last_line = yylineno; \
                       yylloc.last_column = yycolumn + yyleng - 1; \
                       yycolumn += yyleng;


bool isOctal(QString str)
{
    bool ok;

    str.toUInt(&ok, 8);

    return ok;
}

bool isHex(QString str)
{
    bool ok;

    str.toUInt(&ok, 16);

    return ok;
}

QString stringRepToString(QString strRep)
{
    QString str = strRep;
    str.remove(0, 1);
    str.remove(str.length() - 1, 1);
    int start = 0;

    while (true)
    {
        start = str.indexOf('\\', start);
        int next = start + 1;

        if (start < 0 || next >= str.length())
            break;

        if (str[next] == '0')
            str.replace(start, 2, QChar('\0'));
        else if (str[next] == 'a')
            str.replace(start, 2, QChar('\a'));
        else if (str[next] == 'b')
            str.replace(start, 2, QChar('\b'));
        else if (str[next] == 'f')
            str.replace(start, 2, QChar('\f'));
        else if (str[next] == 'n')
            str.replace(start, 2, QChar('\n'));
        else if (str[next] == 'r')
            str.replace(start, 2, QChar('\r'));
        else if (str[next] == 't')
            str.replace(start, 2, QChar('\t'));
        else if (str[next] == 'v')
            str.replace(start, 2, QChar('\v'));
        else if (next + 2 < str.length() && isOctal(str.mid(next, 3)))
            str.replace(start, 4, QChar(str.mid(next, 3).toUInt(NULL, 8)));
        else if (str[next] == 'x' && next + 2 < str.length() && isHex(str.mid(next + 1, 2)))
            str.replace(start, 4, QChar(str.mid(next + 1, 2).toUInt(NULL, 16)));
        else if (str[next] == 'u' && next + 4 < str.length() && isHex(str.mid(next + 1, 4)))
            str.replace(start, 6, QChar(str.mid(next + 1, 4).toUInt(NULL, 16)));
        else if (str[next] == 'U' && next + 8 < str.length() && isHex(str.mid(next + 1, 8)))
            str.replace(start, 10, QChar(str.mid(next + 1, 8).toUInt(NULL, 16)));
        else
            str.remove(start, 1);

        start++;
    }

    return str;
}
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
    *yylval.QVariant_t = stringRepToString(yytext);

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
