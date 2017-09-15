function Component()
{
}

Component.prototype.beginInstallation = function()
{
    component.beginInstallation();
}

Component.prototype.createOperations = function()
{
    component.createOperations();

    component.addOperation("InstallIcons", "@TargetDir@/share/icons" );
    component.addOperation("CreateDesktopEntry",
                            "Webcamoid.desktop",
                            "Name=Webcamoid\n"
                            + "GenericName=Webcam Capture Software\n"
                            + "GenericName[ca]=Programari de Captura de Càmera web\n"
                            + "GenericName[de]=Webcam-Capture-Software\n"
                            + "GenericName[el]=κάμερα συλλαμβάνει το λογισμικό\n"
                            + "GenericName[es]=Programa para Captura de la Webcam\n"
                            + "GenericName[fr]=Logiciel de Capture Webcam\n"
                            + "GenericName[gl]=Programa de Captura de Webcam\n"
                            + "GenericName[it]=Webcam Capture Software\n"
                            + "GenericName[ja]=ウェブカメラのキャプチャソフトウェア\n"
                            + "GenericName[ko]=웹캠 캡처 소프트웨어\n"
                            + "GenericName[pt]=Software de Captura de Webcam\n"
                            + "GenericName[ru]=Веб-камера захвата программного обеспечения\n"
                            + "GenericName[zh_CN]=摄像头捕捉软件\n"
                            + "GenericName[zh_TW]=攝像頭捕捉軟件\n"
                            + "Comment=Take photos and record videos with your webcam\n"
                            + "Comment[ca]=Fer fotos i gravar vídeos amb la seva webcam\n"
                            + "Comment[de]=Maak foto's en video's opnemen met uw webcam\n"
                            + "Comment[el]=Τραβήξτε φωτογραφίες και εγγραφή βίντεο με την κάμερα σας\n"
                            + "Comment[es]=Tome fotos y grabe videos con su camara web\n"
                            + "Comment[fr]=Prenez des photos et enregistrer des vidéos avec votre webcam\n"
                            + "Comment[gl]=Facer fotos e gravar vídeos coa súa cámara web\n"
                            + "Comment[it]=Scatta foto e registrare video con la tua webcam\n"
                            + "Comment[ja]=ウェブカメラで写真や記録ビデオを撮影\n"
                            + "Comment[ko]=웹캠으로 사진과 기록 비디오를 촬영\n"
                            + "Comment[pt]=Tirar fotos e gravar vídeos com sua webcam\n"
                            + "Comment[ru]=Возьмите фотографии и записывать видео с веб-камеры\n"
                            + "Comment[zh_CN]=拍摄照片和录制视频与您的摄像头\n"
                            + "Comment[zh_TW]=拍攝照片和錄製視頻與您的攝像頭\n"
                            + "Keywords=photo;video;webcam;\n"
                            + "Exec=" + installer.value("RunProgram") + "\n"
                            + "Icon=webcamoid\n"
                            + "Terminal=false\n"
                            + "Type=Application\n"
                            + "Categories=AudioVideo;Player;Qt;\n"
                            + "StartupNotify=true\n");
}
