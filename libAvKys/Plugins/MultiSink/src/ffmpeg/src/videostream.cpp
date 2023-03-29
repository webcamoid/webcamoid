/* Webcamoid, webcam capture application.
 * Copyright (C) 2017  Gonzalo Exequiel Pedone
 *
 * Webcamoid is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * Webcamoid is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Webcamoid. If not, see <http://www.gnu.org/licenses/>.
 *
 * Web-Site: http://webcamoid.github.io/
 */

#include <QDebug>
#include <QDateTime>
#include <QImage>
#include <QMutex>
#include <QWaitCondition>
#include <QtMath>
#include <akcaps.h>
#include <akfrac.h>
#include <akpacket.h>
#include <akvideopacket.h>

extern "C"
{
    #include <libavutil/imgutils.h>
    #include <libswscale/swscale.h>
}

#include "videostream.h"
#include "mediawriterffmpeg.h"

using FFToAkFormatMap = QMap<AVPixelFormat, AkVideoCaps::PixelFormat>;

inline const FFToAkFormatMap &initFFToAkFormatMap()
{
    static const FFToAkFormatMap formatMap {
        {AV_PIX_FMT_NONE        , AkVideoCaps::Format_none        },
        {AV_PIX_FMT_YUV420P     , AkVideoCaps::Format_yuv420p     },
        {AV_PIX_FMT_YUYV422     , AkVideoCaps::Format_yuyv422     },
        {AV_PIX_FMT_RGB24       , AkVideoCaps::Format_rgb24       },
        {AV_PIX_FMT_BGR24       , AkVideoCaps::Format_bgr24       },
        {AV_PIX_FMT_YUV422P     , AkVideoCaps::Format_yuv422p     },
        {AV_PIX_FMT_YUV444P     , AkVideoCaps::Format_yuv444p     },
        {AV_PIX_FMT_YUV410P     , AkVideoCaps::Format_yuv410p     },
        {AV_PIX_FMT_YUV411P     , AkVideoCaps::Format_yuv411p     },
        {AV_PIX_FMT_GRAY8       , AkVideoCaps::Format_gray8       },
        {AV_PIX_FMT_UYVY422     , AkVideoCaps::Format_uyvy422     },
        {AV_PIX_FMT_BGR8        , AkVideoCaps::Format_bgr233      },
        {AV_PIX_FMT_RGB8        , AkVideoCaps::Format_rgb332      },
        {AV_PIX_FMT_NV12        , AkVideoCaps::Format_nv12        },
        {AV_PIX_FMT_NV21        , AkVideoCaps::Format_nv21        },
        {AV_PIX_FMT_ARGB        , AkVideoCaps::Format_argb        },
        {AV_PIX_FMT_RGBA        , AkVideoCaps::Format_rgba        },
        {AV_PIX_FMT_ABGR        , AkVideoCaps::Format_abgr        },
        {AV_PIX_FMT_BGRA        , AkVideoCaps::Format_bgra        },
        {AV_PIX_FMT_GRAY16BE    , AkVideoCaps::Format_gray16be    },
        {AV_PIX_FMT_GRAY16LE    , AkVideoCaps::Format_gray16le    },
        {AV_PIX_FMT_YUV440P     , AkVideoCaps::Format_yuv440p     },
        {AV_PIX_FMT_YUVA420P    , AkVideoCaps::Format_yuva420p    },
        {AV_PIX_FMT_RGB48BE     , AkVideoCaps::Format_bgr555le    },
        {AV_PIX_FMT_RGB48LE     , AkVideoCaps::Format_rgb48be     },
        {AV_PIX_FMT_RGB565BE    , AkVideoCaps::Format_rgb48le     },
        {AV_PIX_FMT_RGB565LE    , AkVideoCaps::Format_rgb565be    },
        {AV_PIX_FMT_RGB555BE    , AkVideoCaps::Format_rgb565le    },
        {AV_PIX_FMT_RGB555LE    , AkVideoCaps::Format_rgb555be    },
        {AV_PIX_FMT_BGR565BE    , AkVideoCaps::Format_rgb555le    },
        {AV_PIX_FMT_BGR565LE    , AkVideoCaps::Format_bgr565be    },
        {AV_PIX_FMT_BGR555BE    , AkVideoCaps::Format_bgr565le    },
        {AV_PIX_FMT_BGR555LE    , AkVideoCaps::Format_bgr555be    },
        {AV_PIX_FMT_YUV420P16LE , AkVideoCaps::Format_yuv420p16le },
        {AV_PIX_FMT_YUV420P16BE , AkVideoCaps::Format_yuv420p16be },
        {AV_PIX_FMT_YUV422P16LE , AkVideoCaps::Format_yuv422p16le },
        {AV_PIX_FMT_YUV422P16BE , AkVideoCaps::Format_yuv422p16be },
        {AV_PIX_FMT_YUV444P16LE , AkVideoCaps::Format_yuv444p16le },
        {AV_PIX_FMT_YUV444P16BE , AkVideoCaps::Format_yuv444p16be },
        {AV_PIX_FMT_RGB444LE    , AkVideoCaps::Format_rgb444le    },
        {AV_PIX_FMT_RGB444BE    , AkVideoCaps::Format_rgb444be    },
        {AV_PIX_FMT_BGR444LE    , AkVideoCaps::Format_bgr444le    },
        {AV_PIX_FMT_BGR444BE    , AkVideoCaps::Format_bgr444be    },
        {AV_PIX_FMT_YA8         , AkVideoCaps::Format_graya8      },
        {AV_PIX_FMT_Y400A       , AkVideoCaps::Format_graya8      },
        {AV_PIX_FMT_GRAY8A      , AkVideoCaps::Format_graya8      },
        {AV_PIX_FMT_BGR48BE     , AkVideoCaps::Format_bgr48be     },
        {AV_PIX_FMT_BGR48LE     , AkVideoCaps::Format_bgr48le     },
        {AV_PIX_FMT_YUV420P9BE  , AkVideoCaps::Format_yuv420p9be  },
        {AV_PIX_FMT_YUV420P9LE  , AkVideoCaps::Format_yuv420p9le  },
        {AV_PIX_FMT_YUV420P10BE , AkVideoCaps::Format_yuv420p10be },
        {AV_PIX_FMT_YUV420P10LE , AkVideoCaps::Format_yuv420p10le },
        {AV_PIX_FMT_YUV422P10BE , AkVideoCaps::Format_yuv422p10be },
        {AV_PIX_FMT_YUV422P10LE , AkVideoCaps::Format_yuv422p10le },
        {AV_PIX_FMT_YUV444P9BE  , AkVideoCaps::Format_yuv444p9be  },
        {AV_PIX_FMT_YUV444P9LE  , AkVideoCaps::Format_yuv444p9le  },
        {AV_PIX_FMT_YUV444P10BE , AkVideoCaps::Format_yuv444p10be },
        {AV_PIX_FMT_YUV444P10LE , AkVideoCaps::Format_yuv444p10le },
        {AV_PIX_FMT_YUV422P9BE  , AkVideoCaps::Format_yuv422p9be  },
        {AV_PIX_FMT_YUV422P9LE  , AkVideoCaps::Format_yuv422p9le  },
        {AV_PIX_FMT_GBRP        , AkVideoCaps::Format_gbrp        },
        {AV_PIX_FMT_GBR24P      , AkVideoCaps::Format_gbr24p      },
        {AV_PIX_FMT_GBRP9BE     , AkVideoCaps::Format_gbrp9be     },
        {AV_PIX_FMT_GBRP9LE     , AkVideoCaps::Format_gbrp9le     },
        {AV_PIX_FMT_GBRP10BE    , AkVideoCaps::Format_gbrp10be    },
        {AV_PIX_FMT_GBRP10LE    , AkVideoCaps::Format_gbrp10le    },
        {AV_PIX_FMT_GBRP16BE    , AkVideoCaps::Format_gbrp16be    },
        {AV_PIX_FMT_GBRP16LE    , AkVideoCaps::Format_gbrp16le    },
        {AV_PIX_FMT_YUVA422P    , AkVideoCaps::Format_yuva422p    },
        {AV_PIX_FMT_YUVA444P    , AkVideoCaps::Format_yuva444p    },
        {AV_PIX_FMT_YUVA420P9BE , AkVideoCaps::Format_yuva420p9be },
        {AV_PIX_FMT_YUVA420P9LE , AkVideoCaps::Format_yuva420p9le },
        {AV_PIX_FMT_YUVA422P9BE , AkVideoCaps::Format_yuva422p9be },
        {AV_PIX_FMT_YUVA422P9LE , AkVideoCaps::Format_yuva422p9le },
        {AV_PIX_FMT_YUVA444P9BE , AkVideoCaps::Format_yuva444p9be },
        {AV_PIX_FMT_YUVA444P9LE , AkVideoCaps::Format_yuva444p9le },
        {AV_PIX_FMT_YUVA420P10BE, AkVideoCaps::Format_yuva420p10be},
        {AV_PIX_FMT_YUVA420P10LE, AkVideoCaps::Format_yuva420p10le},
        {AV_PIX_FMT_YUVA422P10BE, AkVideoCaps::Format_yuva422p10be},
        {AV_PIX_FMT_YUVA422P10LE, AkVideoCaps::Format_yuva422p10le},
        {AV_PIX_FMT_YUVA444P10BE, AkVideoCaps::Format_yuva444p10be},
        {AV_PIX_FMT_YUVA444P10LE, AkVideoCaps::Format_yuva444p10le},
        {AV_PIX_FMT_YUVA420P16BE, AkVideoCaps::Format_yuva420p16be},
        {AV_PIX_FMT_YUVA420P16LE, AkVideoCaps::Format_yuva420p16le},
        {AV_PIX_FMT_YUVA422P16BE, AkVideoCaps::Format_yuva422p16be},
        {AV_PIX_FMT_YUVA422P16LE, AkVideoCaps::Format_yuva422p16le},
        {AV_PIX_FMT_YUVA444P16BE, AkVideoCaps::Format_yuva444p16be},
        {AV_PIX_FMT_YUVA444P16LE, AkVideoCaps::Format_yuva444p16le},
        {AV_PIX_FMT_NV16        , AkVideoCaps::Format_nv16        },
        {AV_PIX_FMT_NV20LE      , AkVideoCaps::Format_nv20le      },
        {AV_PIX_FMT_NV20BE      , AkVideoCaps::Format_nv20be      },
        {AV_PIX_FMT_RGBA64BE    , AkVideoCaps::Format_rgba64be    },
        {AV_PIX_FMT_RGBA64LE    , AkVideoCaps::Format_rgba64le    },
        {AV_PIX_FMT_BGRA64BE    , AkVideoCaps::Format_bgra64be    },
        {AV_PIX_FMT_BGRA64LE    , AkVideoCaps::Format_bgra64le    },
        {AV_PIX_FMT_YVYU422     , AkVideoCaps::Format_yvyu422     },
        {AV_PIX_FMT_YA16BE      , AkVideoCaps::Format_graya16be   },
        {AV_PIX_FMT_YA16LE      , AkVideoCaps::Format_graya16le   },
        {AV_PIX_FMT_GBRAP       , AkVideoCaps::Format_gbrap       },
        {AV_PIX_FMT_GBRAP16BE   , AkVideoCaps::Format_gbrap16be   },
        {AV_PIX_FMT_GBRAP16LE   , AkVideoCaps::Format_gbrap16le   },
        {AV_PIX_FMT_0RGB        , AkVideoCaps::Format_0rgb        },
        {AV_PIX_FMT_RGB0        , AkVideoCaps::Format_rgb0        },
        {AV_PIX_FMT_0BGR        , AkVideoCaps::Format_0bgr        },
        {AV_PIX_FMT_BGR0        , AkVideoCaps::Format_bgr0        },
        {AV_PIX_FMT_YUV420P12BE , AkVideoCaps::Format_yuv420p12be },
        {AV_PIX_FMT_YUV420P12LE , AkVideoCaps::Format_yuv420p12le },
        {AV_PIX_FMT_YUV420P14BE , AkVideoCaps::Format_yuv420p14be },
        {AV_PIX_FMT_YUV420P14LE , AkVideoCaps::Format_yuv420p14le },
        {AV_PIX_FMT_YUV422P12BE , AkVideoCaps::Format_yuv422p12be },
        {AV_PIX_FMT_YUV422P12LE , AkVideoCaps::Format_yuv422p12le },
        {AV_PIX_FMT_YUV422P14BE , AkVideoCaps::Format_yuv422p14be },
        {AV_PIX_FMT_YUV422P14LE , AkVideoCaps::Format_yuv422p14le },
        {AV_PIX_FMT_YUV444P12BE , AkVideoCaps::Format_yuv444p12be },
        {AV_PIX_FMT_YUV444P12LE , AkVideoCaps::Format_yuv444p12le },
        {AV_PIX_FMT_YUV444P14BE , AkVideoCaps::Format_yuv444p14be },
        {AV_PIX_FMT_YUV444P14LE , AkVideoCaps::Format_yuv444p14le },
        {AV_PIX_FMT_GBRP12BE    , AkVideoCaps::Format_gbrp12be    },
        {AV_PIX_FMT_GBRP12LE    , AkVideoCaps::Format_gbrp12le    },
        {AV_PIX_FMT_GBRP14BE    , AkVideoCaps::Format_gbrp14be    },
        {AV_PIX_FMT_GBRP14LE    , AkVideoCaps::Format_gbrp14le    },
        {AV_PIX_FMT_YUV440P10LE , AkVideoCaps::Format_yuv440p10le },
        {AV_PIX_FMT_YUV440P10BE , AkVideoCaps::Format_yuv440p10be },
        {AV_PIX_FMT_YUV440P12LE , AkVideoCaps::Format_yuv440p12le },
        {AV_PIX_FMT_YUV440P12BE , AkVideoCaps::Format_yuv440p12be },
        {AV_PIX_FMT_AYUV64LE    , AkVideoCaps::Format_ayuv64le    },
        {AV_PIX_FMT_AYUV64BE    , AkVideoCaps::Format_ayuv64be    },
        {AV_PIX_FMT_P010LE      , AkVideoCaps::Format_p010le      },
        {AV_PIX_FMT_P010BE      , AkVideoCaps::Format_p010be      },
        {AV_PIX_FMT_GBRAP12BE   , AkVideoCaps::Format_gbrap12be   },
        {AV_PIX_FMT_GBRAP12LE   , AkVideoCaps::Format_gbrap12le   },
        {AV_PIX_FMT_GBRAP10BE   , AkVideoCaps::Format_gbrap10be   },
        {AV_PIX_FMT_GBRAP10LE   , AkVideoCaps::Format_gbrap10le   },
        {AV_PIX_FMT_GRAY12BE    , AkVideoCaps::Format_gray12be    },
        {AV_PIX_FMT_GRAY12LE    , AkVideoCaps::Format_gray12le    },
        {AV_PIX_FMT_GRAY10BE    , AkVideoCaps::Format_gray10be    },
        {AV_PIX_FMT_GRAY10LE    , AkVideoCaps::Format_gray10le    },
        {AV_PIX_FMT_P016LE      , AkVideoCaps::Format_p016le      },
        {AV_PIX_FMT_P016BE      , AkVideoCaps::Format_p016be      },
        {AV_PIX_FMT_GRAY9BE     , AkVideoCaps::Format_gray9be     },
        {AV_PIX_FMT_GRAY9LE     , AkVideoCaps::Format_gray9le     },
        {AV_PIX_FMT_GRAY14BE    , AkVideoCaps::Format_gray14be    },
        {AV_PIX_FMT_GRAY14LE    , AkVideoCaps::Format_gray14le    },
        {AV_PIX_FMT_YUVA422P12BE, AkVideoCaps::Format_yuva422p12be},
        {AV_PIX_FMT_YUVA422P12LE, AkVideoCaps::Format_yuva422p12le},
        {AV_PIX_FMT_YUVA444P12BE, AkVideoCaps::Format_yuva444p12be},
        {AV_PIX_FMT_YUVA444P12LE, AkVideoCaps::Format_yuva444p12le},
        {AV_PIX_FMT_NV24        , AkVideoCaps::Format_nv24        },
        {AV_PIX_FMT_NV42        , AkVideoCaps::Format_nv42        },
#if LIBAVUTIL_VERSION_INT >= AV_VERSION_INT(56, 42, 100)
        {AV_PIX_FMT_Y210BE      , AkVideoCaps::Format_y210be      },
        {AV_PIX_FMT_Y210LE      , AkVideoCaps::Format_y210le      },
#endif
#if LIBAVUTIL_VERSION_INT >= AV_VERSION_INT(57, 9, 100)
        {AV_PIX_FMT_P210BE      , AkVideoCaps::Format_p210be      },
        {AV_PIX_FMT_P210LE      , AkVideoCaps::Format_p210le      },
        {AV_PIX_FMT_P410BE      , AkVideoCaps::Format_p410be      },
        {AV_PIX_FMT_P410LE      , AkVideoCaps::Format_p410le      },
        {AV_PIX_FMT_P216BE      , AkVideoCaps::Format_p216be      },
        {AV_PIX_FMT_P216LE      , AkVideoCaps::Format_p216le      },
        {AV_PIX_FMT_P416BE      , AkVideoCaps::Format_p416be      },
        {AV_PIX_FMT_P416LE      , AkVideoCaps::Format_p416le      },
#endif
        {AV_PIX_FMT_RGB32       , AkVideoCaps::Format_argbpack    },
        {AV_PIX_FMT_RGB32_1     , AkVideoCaps::Format_rgbapack    },
        {AV_PIX_FMT_BGR32       , AkVideoCaps::Format_abgrpack    },
        {AV_PIX_FMT_BGR32_1     , AkVideoCaps::Format_bgrapack    },
        {AV_PIX_FMT_0RGB32      , AkVideoCaps::Format_0rgbpack    },
        {AV_PIX_FMT_0BGR32      , AkVideoCaps::Format_0bgrpack    },
        {AV_PIX_FMT_GRAY9       , AkVideoCaps::Format_gray9       },
        {AV_PIX_FMT_GRAY10      , AkVideoCaps::Format_gray10      },
        {AV_PIX_FMT_GRAY12      , AkVideoCaps::Format_gray12      },
        {AV_PIX_FMT_GRAY14      , AkVideoCaps::Format_gray14      },
        {AV_PIX_FMT_GRAY16      , AkVideoCaps::Format_gray16      },
        {AV_PIX_FMT_YA16        , AkVideoCaps::Format_graya16     },
        {AV_PIX_FMT_RGB48       , AkVideoCaps::Format_rgb48       },
        {AV_PIX_FMT_RGB565      , AkVideoCaps::Format_rgb565      },
        {AV_PIX_FMT_RGB555      , AkVideoCaps::Format_rgb555      },
        {AV_PIX_FMT_RGB444      , AkVideoCaps::Format_rgb444      },
        {AV_PIX_FMT_RGBA64      , AkVideoCaps::Format_rgba64      },
        {AV_PIX_FMT_BGR48       , AkVideoCaps::Format_bgr48       },
        {AV_PIX_FMT_BGR565      , AkVideoCaps::Format_bgr565      },
        {AV_PIX_FMT_BGR555      , AkVideoCaps::Format_bgr555      },
        {AV_PIX_FMT_BGR444      , AkVideoCaps::Format_bgr444      },
        {AV_PIX_FMT_BGRA64      , AkVideoCaps::Format_bgra64      },
        {AV_PIX_FMT_YUV420P9    , AkVideoCaps::Format_yuv420p9    },
        {AV_PIX_FMT_YUV422P9    , AkVideoCaps::Format_yuv422p9    },
        {AV_PIX_FMT_YUV444P9    , AkVideoCaps::Format_yuv444p9    },
        {AV_PIX_FMT_YUV420P10   , AkVideoCaps::Format_yuv420p10   },
        {AV_PIX_FMT_YUV422P10   , AkVideoCaps::Format_yuv422p10   },
        {AV_PIX_FMT_YUV440P10   , AkVideoCaps::Format_yuv440p10   },
        {AV_PIX_FMT_YUV444P10   , AkVideoCaps::Format_yuv444p10   },
        {AV_PIX_FMT_YUV420P12   , AkVideoCaps::Format_yuv420p12   },
        {AV_PIX_FMT_YUV422P12   , AkVideoCaps::Format_yuv422p12   },
        {AV_PIX_FMT_YUV440P12   , AkVideoCaps::Format_yuv440p12   },
        {AV_PIX_FMT_YUV444P12   , AkVideoCaps::Format_yuv444p12   },
        {AV_PIX_FMT_YUV420P14   , AkVideoCaps::Format_yuv420p14   },
        {AV_PIX_FMT_YUV422P14   , AkVideoCaps::Format_yuv422p14   },
        {AV_PIX_FMT_YUV444P14   , AkVideoCaps::Format_yuv444p14   },
        {AV_PIX_FMT_YUV420P16   , AkVideoCaps::Format_yuv420p16   },
        {AV_PIX_FMT_YUV422P16   , AkVideoCaps::Format_yuv422p16   },
        {AV_PIX_FMT_YUV444P16   , AkVideoCaps::Format_yuv444p16   },
        {AV_PIX_FMT_GBRP9       , AkVideoCaps::Format_gbrp9       },
        {AV_PIX_FMT_GBRP10      , AkVideoCaps::Format_gbrp10      },
        {AV_PIX_FMT_GBRP12      , AkVideoCaps::Format_gbrp12      },
        {AV_PIX_FMT_GBRP14      , AkVideoCaps::Format_gbrp14      },
        {AV_PIX_FMT_GBRP16      , AkVideoCaps::Format_gbrp16      },
        {AV_PIX_FMT_GBRAP10     , AkVideoCaps::Format_gbrap10     },
        {AV_PIX_FMT_GBRAP12     , AkVideoCaps::Format_gbrap12     },
        {AV_PIX_FMT_GBRAP16     , AkVideoCaps::Format_gbrap16     },
        {AV_PIX_FMT_YUVA420P9   , AkVideoCaps::Format_yuva420p9   },
        {AV_PIX_FMT_YUVA422P9   , AkVideoCaps::Format_yuva422p9   },
        {AV_PIX_FMT_YUVA444P9   , AkVideoCaps::Format_yuva444p9   },
        {AV_PIX_FMT_YUVA420P10  , AkVideoCaps::Format_yuva420p10  },
        {AV_PIX_FMT_YUVA422P10  , AkVideoCaps::Format_yuva422p10  },
        {AV_PIX_FMT_YUVA444P10  , AkVideoCaps::Format_yuva444p10  },
        {AV_PIX_FMT_YUVA422P12  , AkVideoCaps::Format_yuva422p12  },
        {AV_PIX_FMT_YUVA444P12  , AkVideoCaps::Format_yuva444p12  },
        {AV_PIX_FMT_YUVA420P16  , AkVideoCaps::Format_yuva420p16  },
        {AV_PIX_FMT_YUVA422P16  , AkVideoCaps::Format_yuva422p16  },
        {AV_PIX_FMT_YUVA444P16  , AkVideoCaps::Format_yuva444p16  },
        {AV_PIX_FMT_NV20        , AkVideoCaps::Format_nv20        },
        {AV_PIX_FMT_AYUV64      , AkVideoCaps::Format_ayuv64      },
        {AV_PIX_FMT_P010        , AkVideoCaps::Format_p010        },
        {AV_PIX_FMT_P016        , AkVideoCaps::Format_p016        },
#if LIBAVUTIL_VERSION_INT >= AV_VERSION_INT(56, 42, 100)
        {AV_PIX_FMT_Y210        , AkVideoCaps::Format_y210        },
#endif
#if LIBAVUTIL_VERSION_INT >= AV_VERSION_INT(57, 9, 100)
        {AV_PIX_FMT_P210        , AkVideoCaps::Format_p210        },
        {AV_PIX_FMT_P410        , AkVideoCaps::Format_p410        },
        {AV_PIX_FMT_P216        , AkVideoCaps::Format_p216        },
        {AV_PIX_FMT_P416        , AkVideoCaps::Format_p416        },
#endif
    };

    return formatMap;
}

Q_GLOBAL_STATIC_WITH_ARGS(FFToAkFormatMap, ffToAkFormatMap, (initFFToAkFormatMap()))

class VideoStreamPrivate
{
    public:
        AVFrame *m_frame {nullptr};
        SwsContext *m_scaleContext {nullptr};
        QMutex m_frameMutex;
        int64_t m_lastPts {AV_NOPTS_VALUE};
        int64_t m_refPts {AV_NOPTS_VALUE};
        QWaitCondition m_frameReady;
};

VideoStream::VideoStream(const AVFormatContext *formatContext,
                         uint index,
                         int streamIndex,
                         const QVariantMap &configs,
                         const QMap<QString, QVariantMap> &codecOptions,
                         MediaWriterFFmpeg *mediaWriter,
                         QObject *parent):
    AbstractStream(formatContext,
                   index, streamIndex,
                   configs,
                   codecOptions,
                   mediaWriter,
                   parent)
{
    this->d = new VideoStreamPrivate;
    auto codecContext = this->codecContext();
    auto codec = codecContext->codec;
    auto defaultCodecParams = mediaWriter->defaultCodecParams(codec->name);
    codecContext->bit_rate = configs["bitrate"].toInt();

    if (codecContext->bit_rate < 1)
        codecContext->bit_rate = defaultCodecParams["defaultBitRate"].toInt();

    AkVideoCaps videoCaps(configs["caps"].value<AkCaps>());

    auto pixelFormat = videoCaps.format();
    auto supportedPixelFormats =
            defaultCodecParams["supportedPixelFormats"].toList();

    if (!supportedPixelFormats.contains(int(pixelFormat))) {
        auto defaultPixelFormat =
                AkVideoCaps::PixelFormat(defaultCodecParams["defaultPixelFormat"].toInt());
        videoCaps.setFormat(defaultPixelFormat);
    }

    auto supportedFrameRates =
            defaultCodecParams["supportedFrameRates"].toList();

    if (!supportedFrameRates.isEmpty()) {
        AkFrac frameRate;
        qreal maxDiff = std::numeric_limits<qreal>::max();

        for (auto &rate: supportedFrameRates) {
            qreal diff = qAbs(videoCaps.fps().value() - rate.value<AkFrac>().value());

            if (diff < maxDiff) {
                frameRate = rate.value<AkFrac>();

                if (qIsNull(diff))
                    break;

                maxDiff = diff;
            }
        }

        videoCaps.fps() = frameRate;
    }

    switch (codec->id) {
    case AV_CODEC_ID_H261:
        videoCaps = mediaWriter->nearestH261Caps(videoCaps);
        break;
    case AV_CODEC_ID_H263:
        videoCaps = mediaWriter->nearestH263Caps(videoCaps);
        break;
    case AV_CODEC_ID_DVVIDEO:
        videoCaps = mediaWriter->nearestDVCaps(videoCaps);
        break;
    case AV_CODEC_ID_DNXHD:
        videoCaps.setProperty("bitrate", configs["bitrate"]);
        videoCaps = mediaWriter->nearestDNxHDCaps(videoCaps);
        codecContext->bit_rate = videoCaps.property("bitrate").toInt();
        videoCaps.setProperty("bitrate", QVariant());
        break;
    case AV_CODEC_ID_ROQ:
        videoCaps.setWidth(int(qPow(2, qRound(qLn(videoCaps.width()) / qLn(2)))));
        videoCaps.setHeight(int(qPow(2, qRound(qLn(videoCaps.height()) / qLn(2)))));
        videoCaps.fps() = AkFrac(qRound(videoCaps.fps().value()), 1);
        break;
    case AV_CODEC_ID_RV10:
        videoCaps.setWidth(16 * qRound(videoCaps.width() / 16.));
        videoCaps.setHeight(16 * qRound(videoCaps.height() / 16.));
        break;
    case AV_CODEC_ID_AMV:
        videoCaps.setHeight(16 * qRound(videoCaps.height() / 16.));
        break;
    case AV_CODEC_ID_XFACE:
        videoCaps.setWidth(48);
        videoCaps.setHeight(48);
        break;
    default:
        break;
    }

    if (!strcmp(formatContext->oformat->name, "gxf"))
        videoCaps = mediaWriter->nearestGXFCaps(videoCaps);

    codecContext->pix_fmt = ffToAkFormatMap->key(videoCaps.format(),
                                                 AV_PIX_FMT_NONE);
    codecContext->width = videoCaps.width();
    codecContext->height = videoCaps.height();

    auto stream = this->stream();
    AkFrac timeBase = videoCaps.fps().invert();
    stream->time_base.num = int(timeBase.num());
    stream->time_base.den = int(timeBase.den());
    codecContext->time_base = stream->time_base;
    codecContext->gop_size = configs["gop"].toInt();

    if (codecContext->gop_size < 1)
        codecContext->gop_size = defaultCodecParams["defaultGOP"].toInt();
}

VideoStream::~VideoStream()
{
    this->uninit();
    this->deleteFrame(&this->d->m_frame);
    sws_freeContext(this->d->m_scaleContext);
    delete this->d;
}

AkVideoCaps::PixelFormat VideoStream::ffToAkFormat(AVPixelFormat format)
{
    return ffToAkFormatMap->value(format, AkVideoCaps::Format_none);
}

void VideoStream::convertPacket(const AkPacket &packet)
{
    if (!packet)
        return;

    AkVideoPacket videoPacket(packet);
    auto iFormat = ffToAkFormatMap->key(videoPacket.caps().format(), AV_PIX_FMT_NONE);

    if (iFormat == AV_PIX_FMT_NONE)
        return;

    int iWidth = videoPacket.caps().width();
    int iHeight = videoPacket.caps().height();

    if (iWidth < 1 || iHeight < 1)
        return;

    auto codecContext = this->codecContext();

    auto oFrame = av_frame_alloc();
    oFrame->format = codecContext->pix_fmt;
    oFrame->width = codecContext->width;
    oFrame->height = codecContext->height;
    oFrame->pts = packet.pts();

    this->d->m_scaleContext =
            sws_getCachedContext(this->d->m_scaleContext,
                                 iWidth,
                                 iHeight,
                                 iFormat,
                                 oFrame->width,
                                 oFrame->height,
                                 AVPixelFormat(oFrame->format),
                                 SWS_FAST_BILINEAR,
                                 nullptr,
                                 nullptr,
                                 nullptr);

    if (!this->d->m_scaleContext)
        return;

    AVFrame iFrame;
    memset(&iFrame, 0, sizeof(AVFrame));

    for (int plane = 0; plane < videoPacket.planes(); ++plane) {
        iFrame.data[plane] = videoPacket.plane(plane);
        iFrame.linesize[plane] = videoPacket.lineSize(plane);
    }

    if (av_frame_get_buffer(oFrame, 4) < 0)
        return;

    sws_scale(this->d->m_scaleContext,
              iFrame.data,
              iFrame.linesize,
              0,
              iHeight,
              oFrame->data,
              oFrame->linesize);

    this->d->m_frameMutex.lock();
    this->deleteFrame(&this->d->m_frame);
    this->d->m_frame = oFrame;
    this->d->m_frameReady.wakeAll();
    this->d->m_frameMutex.unlock();
}

int VideoStream::encodeData(AVFrame *frame)
{
#ifdef AVFMT_RAWPICTURE
    auto formatContext = this->formatContext();

    if (!frame && formatContext->oformat->flags & AVFMT_RAWPICTURE)
        return AVERROR_EOF;
#endif

    auto codecContext = this->codecContext();

    AkFrac outTimeBase(codecContext->time_base.num,
                       codecContext->time_base.den);

    if (frame) {
        qint64 pts = qRound64(QDateTime::currentMSecsSinceEpoch()
                              / outTimeBase.value()
                              / 1000);

        if (this->d->m_refPts == AV_NOPTS_VALUE)
            this->d->m_lastPts = this->d->m_refPts = pts;
        else if (this->d->m_lastPts != pts)
            this->d->m_lastPts = pts;
        else
            return AVERROR(EAGAIN);

        frame->pts = this->d->m_lastPts - this->d->m_refPts;
    } else {
        this->d->m_lastPts++;
    }

    auto stream = this->stream();

#ifdef AVFMT_RAWPICTURE
    if (formatContext->oformat->flags & AVFMT_RAWPICTURE) {
        // Raw video case - directly store the picture in the packet
        AVPacket pkt;
        av_init_packet(&pkt);
        pkt.flags |= AV_PKT_FLAG_KEY;
        pkt.data = frame? frame->data[0]: nullptr;
        pkt.size = sizeof(AVPicture);
        pkt.pts = frame? frame->pts: this->d->m_lastPts;
        pkt.stream_index = this->streamIndex();

        this->rescaleTS(&pkt, codecContext->time_base, stream->time_base);
        emit this->packetReady(&pkt);

        return 0;
    }
#endif

    // encode the image
    auto result = avcodec_send_frame(codecContext, frame);

    if (result == AVERROR_EOF || result == AVERROR(EAGAIN))
        return result;

    if (result < 0) {
        char errorStr[1024];
        av_strerror(AVERROR(result), errorStr, 1024);
        qDebug() << "Error encoding packets: " << errorStr;

        return result;
    }

    forever {
        auto pkt = av_packet_alloc();
        result = avcodec_receive_packet(codecContext, pkt);

        if (result < 0) {
            av_packet_free(&pkt);

            break;
        }

        pkt->stream_index = this->streamIndex();
        this->rescaleTS(pkt,
                        codecContext->time_base,
                        stream->time_base);

        // Write the compressed frame to the media file.
        emit this->packetReady(pkt);
        av_packet_free(&pkt);
    }

    return result;
}

AVFrame *VideoStream::dequeueFrame()
{
    this->d->m_frameMutex.lock();

    if (!this->d->m_frame)
        if (!this->d->m_frameReady.wait(&this->d->m_frameMutex,
                                        THREAD_WAIT_LIMIT)) {
            this->d->m_frameMutex.unlock();

            return nullptr;
        }

    auto frame = this->d->m_frame;
    this->d->m_frame = nullptr;
    this->d->m_frameMutex.unlock();

    return frame;
}

#include "moc_videostream.cpp"
