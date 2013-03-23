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

%code requires {
#include <QtGui>

#include "pipeline.h"

extern int yylex(void);
void yyerror(Pipeline *pipelineDescription, const char *s);
}

%union {
    QVariant *QVariant_t;
    QString *QString_t;
    QStringList *QStringList_t;
}

%error-verbose
%define parse.lac full
%locations
%parse-param {Pipeline *pipelineDescription}

/* Terminals */
%token <QVariant_t> TOK_INTIGER
%token <QVariant_t> TOK_FLOAT
%token <QVariant_t> TOK_BOOLEAN
%token <QVariant_t> TOK_STRING
%token <QVariant_t> TOK_BITVALUES
%token <QString_t> TOK_IDENTIFIER

/* Operators */
%token TOK_LEFTPAREN
%token TOK_RIGHTPAREN
%token TOK_LEFTCURLYBRACKET
%token TOK_RIGHTCURLYBRACKET
%token TOK_LEFTBRACKET
%token TOK_RIGHTBRACKET
%token TOK_LEFTANGLEBRACKET
%token TOK_RIGHTANGLEBRACKET
%token TOK_SLASH
%token TOK_EXCL
%token TOK_DOT
%token TOK_COMMA
%token TOK_COLON
%token TOK_EQUAL

/* Keywords */
%token TOK_SIZE
%token TOK_SIZEF
%token TOK_POINT
%token TOK_POINTF
%token TOK_RECT
%token TOK_RECTF
%token TOK_LINE
%token TOK_LINEF
%token TOK_DATE
%token TOK_TIME
%token TOK_DATETIME
%token TOK_COLOR
%token TOK_BITS
%token TOK_BYTES
%token TOK_URL
%token TOK_REFIN
%token TOK_REFOUT

/* Types */
%type <QVariant_t> variant
%type <QVariant_t> fraction
%type <QVariant_t> size
%type <QVariant_t> sizeF
%type <QVariant_t> point
%type <QVariant_t> pointF
%type <QVariant_t> rect
%type <QVariant_t> rectF
%type <QVariant_t> line
%type <QVariant_t> lineF
%type <QVariant_t> date
%type <QVariant_t> time
%type <QVariant_t> dateTime
%type <QVariant_t> color
%type <QVariant_t> bits
%type <QVariant_t> bytes
%type <QVariant_t> url
%type <QVariant_t> number
%type <QVariant_t> variantListItems
%type <QVariant_t> variantList
%type <QVariant_t> variantMapPair
%type <QVariant_t> variantMapItems
%type <QVariant_t> variantMap
%type <QStringList_t> signalSlot
%type <QStringList_t> signalSlotLt
%type <QStringList_t> signalSlotGt
%type <QString_t> element
%type <QStringList_t> pipe

%destructor {delete $$;} variant
%destructor {delete $$;} fraction
%destructor {delete $$;} size
%destructor {delete $$;} sizeF
%destructor {delete $$;} point
%destructor {delete $$;} pointF
%destructor {delete $$;} rect
%destructor {delete $$;} rectF
%destructor {delete $$;} line
%destructor {delete $$;} lineF
%destructor {delete $$;} date
%destructor {delete $$;} time
%destructor {delete $$;} dateTime
%destructor {delete $$;} color
%destructor {delete $$;} bits
%destructor {delete $$;} bytes
%destructor {delete $$;} url
%destructor {delete $$;} number
%destructor {delete $$;} variantListItems
%destructor {delete $$;} variantList
%destructor {delete $$;} variantMapPair
%destructor {delete $$;} variantMapItems
%destructor {delete $$;} variantMap
%destructor {delete $$;} signalSlot
%destructor {delete $$;} signalSlotLt
%destructor {delete $$;} signalSlotGt
%destructor {delete $$;} element
%destructor {delete $$;} pipe

%%

start: pipeline {
           if (!pipelineDescription->linkAll())
           {
               yyerror(pipelineDescription, pipelineDescription->error().toUtf8().constData());

               YYABORT;
           }

           if (!pipelineDescription->connectAll())
           {
               yyerror(pipelineDescription, pipelineDescription->error().toUtf8().constData());

               YYABORT;
           }
       }
     ;

pipeline: extendedPipe
        | extendedPipe TOK_COMMA extendedPipe
        ;

extendedPipe: pipe {
                  pipelineDescription->addLinks(*$1);

                  delete $1;
              }
            | TOK_REFIN TOK_EXCL pipe TOK_EXCL TOK_REFOUT {
                  $3->prepend("IN.");
                  $3->append("OUT.");

                  pipelineDescription->addLinks(*$3);

                  delete $3;
              }
            | TOK_REFIN TOK_EXCL pipe {
                  $3->prepend("IN.");

                  pipelineDescription->addLinks(*$3);

                  delete $3;
              }
            | pipe TOK_EXCL TOK_REFOUT {
                  $1->append("OUT.");

                  pipelineDescription->addLinks(*$1);

                  delete $1;
              }
            ;

pipe: element {
          $$ = new QStringList();

          *$$ << *$1;

          delete $1;
      }
    | pipe TOK_EXCL element {
          $$ = $1;

          *$$ << *$3;

          delete $3;
      }
    ;

element: TOK_IDENTIFIER {
             QbElementPtr element = Qb::create(*$1);

             if (!element)
             {
                 yyerror(pipelineDescription, QString("Element '%1' not found").arg(*$1).toUtf8().constData());
                 delete $1;

                 YYABORT;
             }

             QString name = pipelineDescription->addElement(element);
             $$ = new QString();
             *$$ = name;

             delete $1;
         }
       | TOK_IDENTIFIER configs {
             QbElementPtr element = Qb::create(*$1);

             if (!element)
             {
                 yyerror(pipelineDescription, QString("Element '%1' not found").arg(*$1).toUtf8().constData());
                 delete $1;

                 YYABORT;
             }

             QVariantMap properties = pipelineDescription->properties();

             foreach (QString key, properties.keys())
                 element->setProperty(key.toUtf8().constData(),
                                      properties[key]);

             pipelineDescription->resetProperties();
             QString name = pipelineDescription->addElement(element);
             pipelineDescription->solveConnections(name);

             $$ = new QString();
             *$$ = name;

             delete $1;
         }
       | TOK_IDENTIFIER TOK_DOT {
             $$ = $1;
         }
       ;

configs: config
       | configs config
       ;

config: property
      | signalSlot {
            pipelineDescription->setConnections(pipelineDescription->connections() << *$1);

            delete $1;
        }
      ;

signalSlot: signalSlotLt
          | signalSlotGt
          ;

signalSlotLt: TOK_IDENTIFIER TOK_DOT TOK_IDENTIFIER TOK_LEFTANGLEBRACKET TOK_IDENTIFIER {
                 $$ = new QStringList();

                 *$$ << "this?"
                     << *$5
                     << *$1
                     << *$3;

                  delete $1;
                  delete $3;
                  delete $5;
              }
            | TOK_IDENTIFIER TOK_LEFTANGLEBRACKET TOK_IDENTIFIER TOK_DOT TOK_IDENTIFIER {
                  $$ = new QStringList();

                  *$$ << *$3
                      << *$5
                      << "this?"
                      << *$1;

                  delete $1;
                  delete $3;
                  delete $5;
              }
            ;

signalSlotGt: TOK_IDENTIFIER TOK_DOT TOK_IDENTIFIER TOK_RIGHTANGLEBRACKET TOK_IDENTIFIER {
                  $$ = new QStringList();

                  *$$ << *$1
                      << *$3
                      << "this?"
                      << *$5;

                  delete $1;
                  delete $3;
                  delete $5;
              }
            | TOK_IDENTIFIER TOK_RIGHTANGLEBRACKET TOK_IDENTIFIER TOK_DOT TOK_IDENTIFIER {
                  $$ = new QStringList();

                  *$$ << "this?"
                      << *$1
                      << *$3
                      << *$5;

                  delete $1;
                  delete $3;
                  delete $5;
              }
            ;

property: TOK_IDENTIFIER TOK_EQUAL variant {
              QVariantMap properties = pipelineDescription->properties();
              properties[*$1] = *$3;
              pipelineDescription->setProperties(properties);

              delete $1;
              delete $3;
          }
        ;

variant: number
       | TOK_BOOLEAN
       | fraction
       | size
       | sizeF
       | point
       | pointF
       | rect
       | rectF
       | line
       | lineF
       | date
       | time
       | dateTime
       | color
       | bits
       | bytes
       | url
       | TOK_STRING
       | variantList
       | variantMap
       ;

fraction: TOK_INTIGER TOK_SLASH TOK_INTIGER {
              $$ = new QVariant();
              $$->setValue(QbFrac($1->toInt(), $3->toInt()));

              delete $1;
              delete $3;
          }
        ;

rect: TOK_RECT TOK_LEFTPAREN TOK_RIGHTPAREN {$$ = new QVariant(QRect());}
    | TOK_RECT TOK_LEFTPAREN point TOK_COMMA point TOK_RIGHTPAREN {
          $$ = new QVariant();
          *$$ = QRect($3->toPoint(), $5->toPoint());

          delete $3;
          delete $5;
      }
    | TOK_RECT TOK_LEFTPAREN point TOK_COMMA size TOK_RIGHTPAREN {
          $$ = new QVariant();
          *$$ = QRect($3->toPoint(), $5->toSize());

          delete $3;
          delete $5;
      }
    | TOK_RECT TOK_LEFTPAREN number TOK_COMMA number TOK_COMMA number TOK_COMMA number TOK_RIGHTPAREN {
          $$ = new QVariant();
          *$$ = QRect($3->toFloat(), $5->toFloat(), $7->toFloat(), $9->toFloat());

          delete $3;
          delete $5;
          delete $7;
          delete $9;
      }
    ;

rectF: TOK_RECTF TOK_LEFTPAREN TOK_RIGHTPAREN {$$ = new QVariant(QRectF());}
     | TOK_RECTF TOK_LEFTPAREN pointF TOK_COMMA pointF TOK_RIGHTPAREN {
           $$ = new QVariant();
           *$$ = QRectF($3->toPointF(), $5->toPointF());

          delete $3;
          delete $5;
       }
     | TOK_RECTF TOK_LEFTPAREN pointF TOK_COMMA sizeF TOK_RIGHTPAREN {
           $$ = new QVariant();
           *$$ = QRectF($3->toPointF(), $5->toSizeF());

          delete $3;
          delete $5;
       }
     | TOK_RECTF TOK_LEFTPAREN rect TOK_RIGHTPAREN {
           $$ = new QVariant();
           *$$ = QRectF($3->toRect());

          delete $3;
       }
     | TOK_RECTF TOK_LEFTPAREN number TOK_COMMA number TOK_COMMA number TOK_COMMA number TOK_RIGHTPAREN {
           $$ = new QVariant();
           *$$ = QRectF($3->toFloat(), $5->toFloat(), $7->toFloat(), $9->toFloat());

          delete $3;
          delete $5;
          delete $7;
          delete $9;
       }
     ;

line: TOK_LINE TOK_LEFTPAREN TOK_RIGHTPAREN {$$ = new QVariant(QLine());}
    | TOK_LINE TOK_LEFTPAREN point TOK_COMMA point TOK_RIGHTPAREN {
          $$ = new QVariant();
          *$$ = QLine($3->toPoint(), $5->toPoint());

          delete $3;
          delete $5;
      }
    | TOK_LINE TOK_LEFTPAREN number TOK_COMMA number TOK_COMMA number TOK_COMMA number TOK_RIGHTPAREN {
          $$ = new QVariant();
          *$$ = QLine($3->toFloat(), $5->toFloat(), $7->toFloat(), $9->toFloat());

          delete $3;
          delete $5;
          delete $7;
          delete $9;
      }
    ;

lineF: TOK_LINEF TOK_LEFTPAREN TOK_RIGHTPAREN {$$ = new QVariant(QLineF());}
     | TOK_LINEF TOK_LEFTPAREN pointF TOK_COMMA pointF TOK_RIGHTPAREN {
           $$ = new QVariant();
           *$$ = QLineF($3->toPointF(), $5->toPointF());

          delete $3;
          delete $5;
       }
     | TOK_LINEF TOK_LEFTPAREN line TOK_RIGHTPAREN {
           $$ = new QVariant();
           *$$ = QLineF($3->toLine());

          delete $3;
       }
     | TOK_LINEF TOK_LEFTPAREN number TOK_COMMA number TOK_COMMA number TOK_COMMA number TOK_RIGHTPAREN {
           $$ = new QVariant();
           *$$ = QLineF($3->toFloat(), $5->toFloat(), $7->toFloat(), $9->toFloat());

          delete $3;
          delete $5;
          delete $7;
          delete $9;
       }
     ;

point: TOK_POINT TOK_LEFTPAREN TOK_RIGHTPAREN {$$ = new QVariant(QPoint());}
     | TOK_POINT TOK_LEFTPAREN number TOK_COMMA number TOK_RIGHTPAREN {
           $$ = new QVariant();
           *$$ = QPoint($3->toFloat(), $5->toFloat());

          delete $3;
          delete $5;
       }
     ;

pointF: TOK_POINTF TOK_LEFTPAREN TOK_RIGHTPAREN {$$ = new QVariant(QPointF());}
      | TOK_POINTF TOK_LEFTPAREN point TOK_RIGHTPAREN {
            $$ = new QVariant();
            *$$ = QPointF($3->toPoint());

          delete $3;
        }
      | TOK_POINTF TOK_LEFTPAREN number TOK_COMMA number TOK_RIGHTPAREN {
            $$ = new QVariant();
            *$$ = QPointF($3->toFloat(), $5->toFloat());

          delete $3;
          delete $5;
        }
      ;

size: TOK_SIZE TOK_LEFTPAREN TOK_RIGHTPAREN {$$ = new QVariant(QSize());}
    | TOK_SIZE TOK_LEFTPAREN number TOK_COMMA number TOK_RIGHTPAREN {
          $$ = new QVariant();
          *$$ = QSize($3->toFloat(), $5->toFloat());

          delete $3;
          delete $5;
      }
    ;

sizeF: TOK_SIZEF TOK_LEFTPAREN TOK_RIGHTPAREN {$$ = new QVariant(QSizeF());}
     | TOK_SIZEF TOK_LEFTPAREN size TOK_RIGHTPAREN {
           $$ = new QVariant();
           *$$ = QSizeF($3->toSize());

           delete $3;
       }
     | TOK_SIZEF TOK_LEFTPAREN number TOK_COMMA number TOK_RIGHTPAREN {
           $$ = new QVariant();
           *$$ = QSizeF($3->toFloat(), $5->toFloat());

           delete $3;
           delete $5;
       }
     ;

dateTime: TOK_DATETIME TOK_LEFTPAREN TOK_RIGHTPAREN {$$ = new QVariant(QDateTime());}
        | TOK_DATETIME TOK_LEFTPAREN date TOK_RIGHTPAREN {
              $$ = new QVariant();
              *$$ = QDateTime($3->toDate());

              delete $3;
          }
        | TOK_DATETIME TOK_LEFTPAREN date TOK_COMMA time TOK_RIGHTPAREN {
              $$ = new QVariant();
              *$$ = QDateTime($3->toDate(), $5->toTime());

              delete $3;
              delete $5;
          }
        ;

date: TOK_DATE TOK_LEFTPAREN TOK_RIGHTPAREN {$$ = new QVariant(QDate());}
    | TOK_DATE TOK_LEFTPAREN number TOK_COMMA number TOK_COMMA number TOK_RIGHTPAREN {
          $$ = new QVariant();
          *$$ = QDate($3->toFloat(), $5->toFloat(), $7->toFloat());

          delete $3;
          delete $5;
          delete $7;
      }
    ;

time: TOK_TIME TOK_LEFTPAREN TOK_RIGHTPAREN {$$ = new QVariant(QTime());}
    | TOK_TIME TOK_LEFTPAREN number TOK_COMMA number TOK_RIGHTPAREN {
          $$ = new QVariant();
          *$$ = QTime($3->toFloat(), $5->toFloat());

          delete $3;
          delete $5;
      }
    | TOK_TIME TOK_LEFTPAREN number TOK_COMMA number TOK_COMMA number TOK_RIGHTPAREN {
          $$ = new QVariant();
          *$$ = QTime($3->toFloat(), $5->toFloat(), $7->toFloat());

          delete $3;
          delete $5;
          delete $7;
      }
    | TOK_TIME TOK_LEFTPAREN number TOK_COMMA number TOK_COMMA number TOK_COMMA number TOK_RIGHTPAREN {
          $$ = new QVariant();
          *$$ = QTime($3->toFloat(), $5->toFloat(), $7->toFloat(), $9->toFloat());

          delete $3;
          delete $5;
          delete $7;
          delete $9;
      }
    ;

color: TOK_COLOR TOK_LEFTPAREN TOK_RIGHTPAREN {$$ = new QVariant(QColor());}
     | TOK_COLOR TOK_LEFTPAREN TOK_STRING TOK_RIGHTPAREN {
           $$ = new QVariant();
           *$$ = QColor($3->toString());

           delete $3;
       }
     | TOK_COLOR TOK_LEFTPAREN number TOK_COMMA number TOK_COMMA number TOK_RIGHTPAREN {
           $$ = new QVariant();
           *$$ = QColor($3->toFloat(), $5->toFloat(), $7->toFloat());

           delete $3;
           delete $5;
           delete $7;
       }
     | TOK_COLOR TOK_LEFTPAREN number TOK_COMMA number TOK_COMMA number TOK_COMMA number TOK_RIGHTPAREN {
           $$ = new QVariant();
           *$$ = QColor($3->toFloat(), $5->toFloat(), $7->toFloat(), $9->toFloat());

           delete $3;
           delete $5;
           delete $7;
           delete $9;
       }
     ;

bits: TOK_BITS TOK_LEFTPAREN TOK_RIGHTPAREN {$$ = new QVariant(QBitArray());}
    | TOK_BITS TOK_LEFTPAREN TOK_BITVALUES TOK_RIGHTPAREN  {
          $$ = new QVariant();
          *$$ = $3->toBitArray();

          delete $3;
      }
    ;

bytes: TOK_BYTES TOK_STRING  {
           $$ = new QVariant();
           *$$ = $2->toString().toUtf8();

           delete $2;
       }
     ;

url: TOK_URL TOK_LEFTPAREN TOK_RIGHTPAREN {$$ = new QVariant(QUrl());}
   | TOK_URL TOK_LEFTPAREN TOK_STRING TOK_RIGHTPAREN {
         $$ = new QVariant();
         *$$ = QUrl($3->toString());

         delete $3;
     }
   ;

variantList: TOK_LEFTBRACKET TOK_RIGHTBRACKET {$$ = new QVariant(QVariantList());}
           | TOK_LEFTBRACKET variantListItems TOK_RIGHTBRACKET {
                 $$ = new QVariant();
                 *$$ = $2->toList();

                 delete $2;
             }
           ;

variantListItems: variant {
                      $$ = new QVariant();

                      QVariantList variantList;

                      variantList << *$1;

                      *$$ = variantList;

                      delete $1;
                  }
                | variantListItems TOK_COMMA variant {
                      $$ = new QVariant();

                      QVariantList variantList($1->toList());

                      variantList << *$3;

                      *$$ = variantList;

                      delete $1;
                      delete $3;
                  }
                ;

variantMap: TOK_LEFTCURLYBRACKET TOK_RIGHTCURLYBRACKET {$$ = new QVariant(QVariantMap());}
          | TOK_LEFTCURLYBRACKET variantMapItems TOK_RIGHTCURLYBRACKET {
                $$ = new QVariant();
                *$$ = $2->toMap();

                delete $2;
            }
          ;

variantMapItems: variantMapPair {
                     $$ = new QVariant();

                     QVariantMap variantMap;
                     QVariantList pair = $1->toList();

                     variantMap[pair[0].toString()] = pair[1];

                     *$$ = variantMap;

                     delete $1;
                 }
               | variantMapItems TOK_COMMA variantMapPair {
                     $$ = new QVariant();

                     QVariantMap variantMap($1->toMap());
                     QVariantList pair = $3->toList();

                     variantMap[pair[0].toString()] = pair[1];

                     *$$ = variantMap;

                     delete $1;
                     delete $3;
                 }
               ;

variantMapPair: TOK_STRING TOK_COLON variant {
                    $$ = new QVariant();

                    QVariantList variantList;

                    variantList << $1->toString() << *$3;

                    *$$ = variantList;

                    delete $1;
                    delete $3;
                }
              ;

number: TOK_INTIGER
      | TOK_FLOAT
      ;

%%

void yyerror(Pipeline *pipelineDescription, const char *s)
{
    QString error = QString("from(lin: %1, col: %2), to(lin: %3, col: %4): %5")
                        .arg(yylloc.first_line)
                        .arg(yylloc.first_column)
                        .arg(yylloc.last_line)
                        .arg(yylloc.last_column)
                        .arg(s);

    pipelineDescription->setError(error);
}
