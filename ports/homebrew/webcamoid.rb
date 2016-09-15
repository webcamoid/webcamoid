class Webcamoid < Formula
    desc "Webcamoid is a full featured webcam capture application."
    homepage "https://webcamoid.github.io/"
    url "https://github.com/webcamoid/webcamoid/archive/a804b82d488f99feacbb59c50880cc34cac5dc16.zip"
    version "8.0.0"
    sha256 "d6fc13352a0e5f5ab4f910f69a7f4e3bedf0655193f833e3443a0ae6b0142a3d"

    depends_on qt5
    depends_on ffmpeg
    depends_on libuvc => :optional
    depends_on :x11

    def install
        if ENV.compiler == :clang
            spec = "macx-clang"
        else
            spec = "macx-g++"
        end

        ffver = `ffmpeg -version | head -n 1 | awk '{print $3}'`

        args = %W [
            -config release
            -spec #{spec}
            PREFIX=#{prefix}
            FFMPEGINCLUDES=/usr/local/Cellar/ffmpeg/#{ffver}/include
            FFMPEGLIBS=-L/usr/local/Cellar/ffmpeg/#{ffver}/lib
            FFMPEGLIBS+=-lavcodec
            FFMPEGLIBS+=-lavdevice
            FFMPEGLIBS+=-lavformat
            FFMPEGLIBS+=-lavutil
            FFMPEGLIBS+=-lswresample
            FFMPEGLIBS+=-lswscale
        ]

        system "qmake", "Webcamoid.pro", *args
        system "make"
        system "make", "install"
    end

    test do
        ENV["DYLD_LIBRARY_PATH"] = "#{lib}"
        system "#{libexec}/webcamoid", "--version"
    end
end
