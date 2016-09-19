class Webcamoid < Formula
    desc "Webcamoid is a full featured webcam capture application."
    homepage "https://webcamoid.github.io/"
    url "https://github.com/webcamoid/webcamoid/archive/43f71ff0420d89dc27b236b0e52fa37fb4af7b72.zip"
    version "8.0.0"
    sha256 "b8ee97a40a3d6c6a522b5e544c67af2dacae0a606b341716cacc62a21d90b68b"

    depends_on "qt5"
    depends_on "ffmpeg"
    depends_on "libuvc" => :optional

    def install
        if ENV.compiler == :clang
            spec = "macx-clang"
        else
            spec = "macx-g++"
        end

        ffver = `ffmpeg -version | head -n 1 | awk '{print $3}'`

        args = %W[
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
