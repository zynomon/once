#include <QApplication>
#include <QWidget>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QLabel>
#include <QPushButton>
#include <QProgressBar>
#include <QTextEdit>
#include <QCheckBox>
#include <QStackedWidget>
#include <QScrollArea>
#include <QProcess>
#include <QMessageBox>
#include <QTimer>
#include <QKeyEvent>
#include <QIcon>
#include <QPainter>
#include <QPainterPath>
#include <QScreen>
#include <QGuiApplication>
#include <QSet>
#include <QMap>
#include <QClipboard>
#include <QMainWindow>
#include <QMenuBar>
#include <QStandardPaths>
#include <QDir>
#include <QFile>
#include <QScrollBar>
#include <QTreeWidget>
#include <QTreeWidgetItem>
#include <QPropertyAnimation>
#include <QEasingCurve>

static const char* QSS = R"QSS(
QTextEdit {
    background-color: #000;
    color: #0f8;
    font-family: 'Nimbus Mono PS', monospace;
    font-size: 13px;
    border: none;
    padding: 12px;
}

QProgressBar {
    background: #252525;
    border: none;
    border-radius: 4px;
}
QProgressBar::chunk {
    border-radius: 4px;
    background: qlineargradient(x1:0, x2:1,
        stop:0.0  #555,
        stop:0.7  #bbb,
        stop:0.9  #fff,
        stop:1.0  #fff);
}

QPushButton#viewLicense,
QPushButton#okBtn {
    background: #5a6fff;
    color: white;
    font-weight: bold;
}
QPushButton#viewLicense { border-radius: 10px; font-size: 16px; }
QPushButton#okBtn       { padding: 8px 24px; font-size: 14px; }

QPushButton#copyBtn { padding: 8px 16px; font-size: 12px; }

QPushButton#closeBtn {
    background: transparent;
    color: #ff5555;
    font-size: 20px;
    border: none;
}
QPushButton#closeBtn:hover {
    background: rgba(255,85,85,100);
    border-radius: 20px;
}

QPushButton#backBtn,
QPushButton#nextBtn {
    border-radius: 8px;
    font-size: 15px;
}
QPushButton#backBtn { background: #2f2f2f; color: #c0c0c0; }
QPushButton#backBtn:disabled { background: #1a1a1a; color: #555; }

QPushButton#nextBtn {
    background: qlineargradient(y1:0, y2:1, stop:0 #6c7fff, stop:1 #5a6fff);
    color: white;
    font-weight: bold;
}
QPushButton#nextBtn:disabled { background: #3a3a3a; color: #888; }

QPushButton#suiteBtn {
    background: rgba(40,40,40,200);
    border: 1px solid #2a2a2a;
    border-radius: 10px;
    padding: 6px;
}
QPushButton#suiteBtn:hover   { background: rgba(60,60,60,220); }
QPushButton#suiteBtn:checked { background: rgba(120,120,120,120); border: 1px solid #aaa; }
QPushButton#suiteBtn:disabled{ background: rgba(25,25,25,150); }
QPushButton#suiteBtn QLabel  { color: #d0d0d0; font-size: 12px; font-weight: bold; }

QLabel#pageTitle,
QLabel#headerTitle,
QLabel#welcome,
QLabel#welcomeSub,
QLabel#stepTitle {
    font-weight: bold;
    color: #e0e0e0;
}
QLabel#pageTitle   { font-size: 28px; }
QLabel#headerTitle { font-size: 20px; }
QLabel#welcome     { font-size: 42px; }
QLabel#welcomeSub  { font-size: 38px; }
QLabel#stepTitle   { font-size: 34px; }

QLabel#welcomeDesc,
QLabel#stepDesc,
QLabel#shortcutHint {
    color: #b0b0b0;
}
QLabel#welcomeDesc  { font-size: 18px; }
QLabel#stepDesc     { font-size: 16px; }
QLabel#shortcutHint { font-size: 10px; }
)QSS";
namespace {
bool Requested = false;
}
class OnboardingTour;
class LicenseViewer;

class TerminalWidget : public QTextEdit {
    Q_OBJECT
public:
    explicit TerminalWidget(QWidget* parent = nullptr) : QTextEdit(parent) {
        setReadOnly(true);
        timer = new QTimer(this);
        connect(timer, &QTimer::timeout, this, &TerminalWidget::tick);
    }

    void typeText(const QString& text) {
        timer->stop();
        fullText = text;
        currentIndex = 0;
        timer->setInterval(60);
        timer->start();
    }

    void clearTerminal() {
        timer->stop();
        clear();
        fullText.clear();
        currentIndex = 0;
    }

    void stopTyping() { timer->stop(); }

signals:
    void typingFinished();

private slots:
    void tick() {
        if (currentIndex < fullText.length()) {
            float progress = (float)currentIndex / fullText.length();
            int interval;
            int charsPerTick;

            if (progress < 0.1)      { interval = 50; charsPerTick = 2;  }
            else if (progress < 0.3) { interval = 30; charsPerTick = 5;  }
            else if (progress < 0.6) { interval = 10; charsPerTick = 15; }
            else                     { interval = 1;  charsPerTick = 30; }

            QTextCursor c = textCursor();
            c.movePosition(QTextCursor::End);
            if (c.positionInBlock() > 0) {
                QTextCursor peek = c;
                peek.movePosition(QTextCursor::Left, QTextCursor::KeepAnchor);
                if (peek.selectedText() == "_")
                    peek.removeSelectedText();
            }
            c.movePosition(QTextCursor::End);
            c.insertText(fullText.mid(currentIndex, charsPerTick));
            c.insertText("_");
            setTextCursor(c);
            verticalScrollBar()->setValue(verticalScrollBar()->maximum());

            currentIndex += charsPerTick;
            if (currentIndex > fullText.length()) currentIndex = fullText.length();
            timer->setInterval(interval);
        } else {
            QTextCursor c = textCursor();
            c.movePosition(QTextCursor::End);
            if (c.positionInBlock() > 0) {
                QTextCursor peek = c;
                peek.movePosition(QTextCursor::Left, QTextCursor::KeepAnchor);
                if (peek.selectedText() == "_")
                    peek.removeSelectedText();
            }
            timer->stop();
            emit typingFinished();
        }
    }

private:
    QTimer* timer = nullptr;
    QString fullText;
    int currentIndex = 0;
};

class LicenseViewer : public QMainWindow {
    Q_OBJECT
public:
    explicit LicenseViewer(QWidget* parent = nullptr)
        : QMainWindow(parent) {
        setWindowTitle("License Agreement");
        resize(760, 520);
        setWindowModality(Qt::ApplicationModal);
        setWindowFlags(windowFlags() & ~Qt::WindowMinimizeButtonHint & ~Qt::WindowCloseButtonHint);

        QFile f(":/LICENSE");
        if (f.open(QIODevice::ReadOnly | QIODevice::Text)) {
            fullText = QString::fromUtf8(f.readAll());
            f.close();
        } else {
            fullText = "ERROR: could not load :/LICENSE from resources.";
        }

        auto* central = new QWidget(this);
        auto* layout = new QVBoxLayout(central);

        auto* scrollArea = new QScrollArea(this);
        scrollArea->setWidgetResizable(true);

        textEdit = new TerminalWidget(this);
        scrollArea->setWidget(textEdit);

        auto* btnLayout = new QHBoxLayout();

        auto* copyBtn = new QPushButton("Copy to Clipboard", this);
        copyBtn->setObjectName("copyBtn");
        connect(copyBtn, &QPushButton::clicked, this, [this]() {
            QGuiApplication::clipboard()->setText(fullText);
        });

        okBtn = new QPushButton("OK", this);
        okBtn->setObjectName("okBtn");
        okBtn->setVisible(false);
        connect(okBtn, &QPushButton::clicked, this, &LicenseViewer::close);

        btnLayout->addWidget(copyBtn);
        btnLayout->addStretch();
        btnLayout->addWidget(okBtn);

        auto* titleLabel = new QLabel("<h3 style='color: #e0e0ff; text-align: center;'>Terms of error</h3>");
        layout->addWidget(titleLabel);
        layout->addWidget(scrollArea, 1);
        layout->addLayout(btnLayout);

        setCentralWidget(central);

        connect(textEdit, &TerminalWidget::typingFinished, this, [this]() {
            okBtn->setVisible(true);
        });

        textEdit->typeText(fullText);
    }

signals:
    void finished();

protected:
    void closeEvent(QCloseEvent* event) override {
        textEdit->stopTyping();
        emit finished();
        event->accept();
    }

private:
    TerminalWidget* textEdit = nullptr;
    QPushButton* okBtn = nullptr;
    QString fullText;
};

class OnboardingTour : public QWidget {
    Q_OBJECT

    struct PkgEntry {
        QString display;
        QString cmd;
        int type;
    };

    struct Profile {
        QString name;
        QString icon;
        QString desc;
        QList<PkgEntry> packages;
    };

    QStackedWidget* stack = nullptr;
    QPushButton *nextBtn = nullptr, *backBtn = nullptr;
    QProgressBar* progress = nullptr;
    QPropertyAnimation* progressAnim = nullptr;
    QMap<QString, Profile> profiles;
    QMap<QString, QPushButton*> suiteButtons;
    QMap<QString, QTreeWidgetItem*> profileItems;
    QTreeWidget* tree = nullptr;
    TerminalWidget* cmdView = nullptr;
    int step = 0;
    bool winKeyPressed = false;
    bool licenseOK = false;
    bool updatingTree = false;
    QString targetCmd;

public:
    OnboardingTour(QWidget* parent = nullptr) : QWidget(parent) {
        setWindowTitle("error.os Setup");
        setWindowFlags(Qt::Window | Qt::FramelessWindowHint);
        setAttribute(Qt::WA_TranslucentBackground);
        setWindowState(Qt::WindowFullScreen);
        setWindowIcon(QIcon::fromTheme("error.os"));
        QScreen* screen = QGuiApplication::primaryScreen();
        setGeometry(screen->geometry());

        setupProfiles();
        setupUI();
        installEventFilter(this);
        showStep(0);
    }

    void paintEvent(QPaintEvent*) override {
        QPainter p(this);
        p.setRenderHint(QPainter::Antialiasing);

        int margin = 20;
        QRect inner = rect().adjusted(margin, margin, -margin, -margin);

        QPainterPath path;
        path.addRoundedRect(inner, 20, 20);

          p.fillPath(path, QColor(0, 0, 0, 160));
    }

    bool eventFilter(QObject*, QEvent* e) override {
        if (step == 1 && e->type() == QEvent::KeyPress) {
            QKeyEvent* ke = static_cast<QKeyEvent*>(e);
            if (ke->key() == Qt::Key_Meta || ke->key() == Qt::Key_Super_L) {
                winKeyPressed = true;
                nextBtn->setEnabled(true);
                return true;
            }
        }
        return false;
    }

private:
    void setupProfiles() {
        profiles["minimal"] = {
            "Minimal", "edit-delete",
            "Stripped down current system dont choose other options if you have very low storage only choose this",
            {
                {"Cleanup script - remove bloat and install more small programs",
                 "sudo apt remove --allow-remove-essentials konsole plasma-discover "
                 "plasma-discover-backend-fwupd kde-spectacle kdeconnect plasma-firewall pipewire-pulse "
                 "plasma-workspace qt6-style-kvantum dolphin && sudo apt autoremove && "
                 "sudo apt install --no-install-recommends zutty spacefm", 0}
            }
        };

        profiles["essential"] = {
            "Essential", "applications-internet",
            "Browser, media player and image viewer - Daily use basics",
            {
                {"Firefox ESR - the web browser",                    "firefox-esr", 1},
                {"mpv - lightweight media player",                   "mpv",         1},
                {"Qimgv - fast image viewer",                        "qimgv",       1},
                {"Ark - archive manager",                            "ark",         1},
                {"Okular - document and PDF reader",                 "okular",      1},
                {"Flatpak setup - Discover backend and Flatseal",    "flatpak plasma-discover-backend-flatpak flatseal", 1},
                {"Flathub remote - enables Flathub app installs",    "flatpak remote-add --if-not-exists flathub https://dl.flathub.org/repo/flathub.flatpakrepo", 0},
                {"Documentation extras - man-db and text-to-speech", "man-db espeak-ng speech-dispatcher speech-dispatcher-espeak-ng", 1}
            }
        };

        profiles["gaming"] = {
            "Gaming", "applications-games",
            "Native Linux games - Not recommended for low-end devices",
            {
                {"GL-117 - 3D flight simulator",       "gl-117",       1},
                {"Pipewalker - puzzle pipe connector", "pipewalker",   1},
                {"SuperTuxKart - kart racing game",    "supertuxkart", 1},
                {"SuperTux - platformer game",         "supertux",     1},
                {"Steam and Bottles - needs Essential's Flatpak, ~500 MB",
                 "which flatpak >/dev/null 2>&1 && flatpak remote-add --if-not-exists flathub https://dl.flathub.org/repo/flathub.flatpakrepo && flatpak install flathub com.valvesoftware.Steam com.usebottles.bottles -y", 0}
            }
        };

        profiles["dev"] = {
            "Developer", "applications-development",
            "Programming tools",
            {
                {"Git - version control",                           "git",             1},
                {"Dolphin VCS plugin - Git status in file manager", "libdolphinvcs6",  1},
                {"Make - build automation tool",                    "make",            1},
                {"Build essentials - compiler toolchain base",      "build-essential", 1},
                {"CMake and Qt GUI - build system and frontend",    "cmake cmake-data cmake-extras cmake-qt-gui", 1},
                {"Lokalize - translation editor",                   "lokalize",        1},
                {"VS Code - Microsoft's editor via .deb, ~100 MB",
                 "wget -O vscode.deb 'https://code.visualstudio.com/sha/download?build=stable&os=linux-deb-x64' "
                 "&& sudo dpkg -i vscode.deb || sudo apt install -f && rm vscode.deb", 0}
            }
        };

        profiles["general"] = {
            "General", "user",
            "For those who wants a general experience.",
            {
                {"KMag - screen magnifier",                        "kmag",            1},
                {"Timeshift - system snapshot and restore",        "timeshift",       1},
                {"KMouseTool - mouse click assist",                "kmousetool",      1},
                {"BleachBit - disk and cache cleaner",             "bleachbit",       1},
                {"Crow Translate - translator with OCR",           "crow-translate",  1},
                {"KMouth - speech synthesizer frontend",           "kmouth",          1},
                {"VokoscreenNG - screen recorder",                 "vokoscreen-ng",   1},
                {"Debian goodies - extra admin tools",             "debian-goodies",  1},
                {"Qmmp - audio player",                            "qmmp",            1},
                {"K3b - disc burning suite",                       "k3b",             1},
                {"KDE Partition Manager - disk partitioning tool", "partitionmanager", 1}
            }
        };

        profiles["designer"] = {
            "Graphic & Arts", "applications-graphics",
            "Photo, video and 3D Editing, 2D Animation and more.",
            {
                {"Inkscape - vector graphics editor",   "inkscape",  1},
                {"Kdenlive - video editor",             "kdenlive",  1},
                {"Blender - 3D modeling and animation", "blender",   1},
                {"Krita - digital painting",            "krita",     1},
                {"Scribus - desktop publishing",        "scribus",   1},
                {"PixiEditor - pixel art editor via .deb, ~80 MB",
                 "wget -O pixieditor.deb 'https://github.com/PixiEditor/PixiEditor/releases/download/2.1.2.4/PixiEditor-2.1.2.4-amd64-linux.deb' "
                 "&& sudo dpkg -i pixieditor.deb || sudo apt install -f && rm pixieditor.deb", 0}
            }
        };

        profiles["server"] = {
            "File Server", "network-server",
            "Samba file sharing setup",
            {
                {"Samba - SMB/CIFS file sharing server",          "samba",                  1},
                {"Samba common binaries - SMB utilities",         "samba-common-bin",       1},
                {"KDE network sharing - Dolphin integration",     "kdenetwork-filesharing", 1},
                {"Dolphin plugins - extra file manager features", "dolphin-plugins",        1},
                {"Smb4K - SMB/CIFS network browser",              "smb4k",                  1}
            }
        };

        profiles["student"] = {
            "Student", "applications-education",
            "Tools to understand more.",
            {
                {"LibreOffice - office suite",             "libreoffice",  1},
                {"Chromium - web browser",                 "chromium",     1},
                {"Thunderbird - email client",             "thunderbird",  1},
                {"Qalculate - advanced calculator",        "qalculate-qt", 1},
                {"Avogadro - molecular editor",            "avogadro",     1},
                {"FreeCAD - parametric 3D CAD",            "freecad",      1},
                {"Thonny - beginner Python IDE",           "thonny",       1},
                {"XaoS - fractal zoomer",                  "xaos",         1},
                {"Tipp10 - touch typing trainer",          "tipp10",       1},
                {"GoldenDict-ng - offline dictionaries",   "goldendict-ng",1},
                {"SimulIDE - real-time circuit simulator", "simulide",     1}
            }
        };

        profiles["cyber"] = {
            "Cyber security", "nethack",
            "For security experts, bugcatchers and exploiters",
            {
                {"Nmap - network scanner",              "nmap",        1},
                {"Wireshark - packet analyzer",         "wireshark",   1},
                {"John the Ripper - password cracker",  "john",        1},
                {"Hydra - login brute forcer",          "hydra",       1},
                {"SQLMap - SQL injection tool",         "sqlmap",      1},
                {"Aircrack-ng - Wi-Fi security suite",  "aircrack-ng", 1},
                {"Hashcat - GPU password recovery",     "hashcat",     1},
                {"Gobuster - directory and DNS busting", "gobuster",   1},
                {"Metasploit - exploitation framework (Kali-only, Rapid7 installer, ~300 MB)",
                 "curl -fsSL https://raw.githubusercontent.com/rapid7/metasploit-omnibus/master/config/templates/"
                 "metasploit-framework-wrappers/msfupdate.erb > /tmp/msfinstall && "
                 "chmod +x /tmp/msfinstall && sudo /tmp/msfinstall && rm /tmp/msfinstall", 0},
                {"Burp Suite Community - web security proxy (PortSwigger installer, ~330 MB)",
                 "wget -O /tmp/burpsuite.sh 'https://portswigger.net/burp/releases/download?product=community&type=Linux' && "
                 "chmod +x /tmp/burpsuite.sh && sudo /tmp/burpsuite.sh -c && rm /tmp/burpsuite.sh", 0}
            }
        };
    }
  void setupUI() {
        QVBoxLayout* main = new QVBoxLayout(this);
        main->setContentsMargins(40, 40, 40, 40);

        QHBoxLayout* header = new QHBoxLayout();
        QLabel* title = new QLabel("error.os setup");
        title->setObjectName("headerTitle");
        QPushButton* close = new QPushButton(QIcon::fromTheme("window-close"), "");
        close->setObjectName("closeBtn");
        close->setFixedSize(40, 40);
        connect(close, &QPushButton::clicked, this, &OnboardingTour::handleClose);
        header->addWidget(title);
        header->addStretch();
        header->addWidget(close);

        progress = new QProgressBar();
        progress->setRange(0, 100);
        progress->setTextVisible(false);
        progress->setFixedHeight(4);

        progressAnim = new QPropertyAnimation(progress, "value", this);
        progressAnim->setDuration(350);
        progressAnim->setEasingCurve(QEasingCurve::InOutCubic);

        stack = new QStackedWidget();
        createSteps();

        QHBoxLayout* footer = new QHBoxLayout();
        backBtn = new QPushButton("Back");
        backBtn->setObjectName("backBtn");
        nextBtn = new QPushButton("Next");
        nextBtn->setObjectName("nextBtn");
        backBtn->setMinimumSize(120, 45);
        nextBtn->setMinimumSize(140, 45);
        connect(backBtn, &QPushButton::clicked, this, &OnboardingTour::handleBack);
        connect(nextBtn, &QPushButton::clicked, this, &OnboardingTour::handleNext);
        footer->addWidget(backBtn);
        footer->addStretch();
        footer->addWidget(nextBtn);

        main->addLayout(header);
        main->addWidget(progress);
        main->addWidget(stack, 1);
        main->addLayout(footer);
    }

    void createSteps() {
        QWidget* s0 = new QWidget();
        QVBoxLayout* l0 = new QVBoxLayout(s0);
        l0->setAlignment(Qt::AlignCenter);
        l0->setSpacing(30);
        QLabel* icon0 = new QLabel();
        icon0->setPixmap(QIcon::fromTheme("error.os").pixmap(148, 148));
        icon0->setAlignment(Qt::AlignCenter);
        QLabel* t0 = new QLabel("Welcome to error.os");
        t0->setObjectName("welcome");
        QLabel* e0 = new QLabel("Neospace  2026");
        e0->setObjectName("welcomeSub");
        e0->setAlignment(Qt::AlignCenter);
        QLabel* d0 = new QLabel("Let's set up your system in a few simple steps");
        d0->setObjectName("welcomeDesc");
        d0->setAlignment(Qt::AlignCenter);
        l0->addWidget(icon0);
        l0->addWidget(t0);
        l0->addWidget(e0);
        l0->addWidget(d0);
        stack->addWidget(s0);

        QWidget* s1 = new QWidget();
        QVBoxLayout* l1 = new QVBoxLayout(s1);
        l1->setAlignment(Qt::AlignCenter);
        l1->setSpacing(30);
        QLabel* icon1 = new QLabel();
        icon1->setPixmap(QIcon::fromTheme("input-keyboard").pixmap(96, 96));
        icon1->setAlignment(Qt::AlignCenter);
        QLabel* t1 = new QLabel("Press the WIN Key");
        t1->setObjectName("stepTitle");
        t1->setAlignment(Qt::AlignCenter);
        QLabel* d1 = new QLabel("This is how you open your application launcher\n(This key is also called Meta or Super key in Linux)");
        d1->setObjectName("stepDesc");
        d1->setAlignment(Qt::AlignCenter);
        QLabel* w2 = new QLabel("here are few shortcuts if you seek to know,\n meta+w - meta+d - meta+e .. check settings for more");
        w2->setObjectName("shortcutHint");
        w2->setAlignment(Qt::AlignCenter);
        l1->addWidget(icon1);
        l1->addWidget(t1);
        l1->addWidget(d1);
        l1->addWidget(w2);
        stack->addWidget(s1);

        QWidget* s2 = new QWidget();
        QVBoxLayout* l2 = new QVBoxLayout(s2);
        l2->setAlignment(Qt::AlignCenter);
        l2->setSpacing(25);
        QLabel* icon2 = new QLabel();
        icon2->setPixmap(QIcon::fromTheme(":/help.png").pixmap(120, 120));
        icon2->setAlignment(Qt::AlignCenter);
        QLabel* t2 = new QLabel("License Agreement");
        t2->setObjectName("stepTitle");
        t2->setAlignment(Qt::AlignCenter);
        QLabel* d2 = new QLabel("Review to go ahead");
        d2->setObjectName("stepDesc");
        d2->setAlignment(Qt::AlignCenter);

        QPushButton* viewLicense = new QPushButton("View License");
        viewLicense->setObjectName("viewLicense");
        viewLicense->setMinimumSize(200, 50);
        connect(viewLicense, &QPushButton::clicked, [this]() {
            LicenseViewer* viewer = new LicenseViewer(this);
            viewer->setAttribute(Qt::WA_DeleteOnClose);
            connect(viewer, &LicenseViewer::finished, this, [this]() {
                licenseOK = true;
                nextBtn->setEnabled(true);
            });
            viewer->show();
        });

        l2->addWidget(icon2);
        l2->addWidget(t2);
        l2->addWidget(d2);
        l2->addWidget(viewLicense);
        stack->addWidget(s2);

        QWidget* s3 = new QWidget();
        QVBoxLayout* l3 = new QVBoxLayout(s3);
        l3->setSpacing(15);

        QLabel* t3 = new QLabel("Choose Your Packages according to your needs,");
        t3->setObjectName("pageTitle");
        l3->addWidget(t3);

        QHBoxLayout* columns = new QHBoxLayout();
        columns->setSpacing(15);

        QVBoxLayout* leftCol = new QVBoxLayout();
        leftCol->setSpacing(12);

        QGridLayout* grid = new QGridLayout();
        grid->setSpacing(8);
        int row = 0, col = 0;
        for (auto it = profiles.begin(); it != profiles.end(); ++it) {
            const QString id = it.key();
            const Profile& p = it.value();
            QPushButton* btn = new QPushButton();
            btn->setObjectName("suiteBtn");
            btn->setToolTip(p.name + " - " + p.desc);
            btn->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
            btn->setMinimumSize(90, 90);
            btn->setCheckable(true);

            QVBoxLayout* btnLayout = new QVBoxLayout(btn);
            btnLayout->setContentsMargins(4, 4, 4, 4);
            btnLayout->setSpacing(4);
            btnLayout->setAlignment(Qt::AlignCenter);

            QLabel* iconLbl = new QLabel();
            iconLbl->setPixmap(QIcon::fromTheme(p.icon).pixmap(40, 40));
            iconLbl->setAlignment(Qt::AlignCenter);
            iconLbl->setAttribute(Qt::WA_TransparentForMouseEvents);

            QLabel* nameLbl = new QLabel(p.name);
            nameLbl->setAlignment(Qt::AlignCenter);
            nameLbl->setWordWrap(true);
            nameLbl->setAttribute(Qt::WA_TransparentForMouseEvents);

            btnLayout->addWidget(iconLbl);
            btnLayout->addWidget(nameLbl);

            suiteButtons.insert(id, btn);
            grid->addWidget(btn, row, col);
            if (++col == 3) { col = 0; ++row; }

            connect(btn, &QPushButton::clicked, this, [this, id](bool checked) {
                if (updatingTree) return;
                setProfileChecked(id, checked);
            });
        }
        leftCol->addLayout(grid, 1);

        cmdView = new TerminalWidget();
        cmdView->setMinimumHeight(110);
        leftCol->addWidget(cmdView);

        columns->addLayout(leftCol, 1);

        tree = new QTreeWidget();
        tree->setColumnCount(1);
        tree->setHeaderHidden(true);
        connect(tree, &QTreeWidget::itemChanged, this, &OnboardingTour::onTreeItemChanged);
        columns->addWidget(tree, 1);

        l3->addLayout(columns, 1);

        stack->addWidget(s3);

        buildTree();

        QWidget* s4 = new QWidget();
        QVBoxLayout* l4 = new QVBoxLayout(s4);
        l4->setAlignment(Qt::AlignCenter);
        l4->setSpacing(30);
        QLabel* icon4 = new QLabel();
        icon4->setPixmap(QIcon::fromTheme("dialog-ok").pixmap(96, 96));
        icon4->setAlignment(Qt::AlignCenter);

        QLabel* t4 = new QLabel("All Set!");
        t4->setObjectName("welcome");
        t4->setAlignment(Qt::AlignCenter);

        QLabel* d4 = new QLabel();
        d4->setObjectName("stepDesc");
        d4->setAlignment(Qt::AlignCenter);
        d4->setWordWrap(true);

        Requested ? d4->setText("Setup is complete! "
        "A TERMINAL will open to install the packages, give it your password - that you used to login and Thats all enjoy error.os :)")
                  :
            d4->setText("Setup is complete! that's it enjoy error.os!");

        QLabel* sh4 = new QLabel("This application will close itself after you press Finish.");
        sh4->setObjectName("shortcutHint");
        sh4->setAlignment(Qt::AlignCenter);
        l4->addWidget(icon4);
        l4->addWidget(t4);
        l4->addWidget(d4);
        l4->addWidget(sh4);
        stack->addWidget(s4);
    }

    void buildTree() {
        updatingTree = true;
        for (auto it = profiles.begin(); it != profiles.end(); ++it) {
            const QString id = it.key();
            const Profile& p = it.value();

            QTreeWidgetItem* top = new QTreeWidgetItem(tree, QStringList{p.name});
            top->setFlags(top->flags() | Qt::ItemIsUserCheckable);
            top->setCheckState(0, Qt::Unchecked);
            top->setData(0, Qt::UserRole, id);
            top->setIcon(0, QIcon::fromTheme(p.icon));

            for (int i = 0; i < p.packages.size(); ++i) {
                const PkgEntry& e = p.packages[i];
                QTreeWidgetItem* leaf = new QTreeWidgetItem(top, QStringList{e.display});
                leaf->setFlags(leaf->flags() | Qt::ItemIsUserCheckable);
                leaf->setCheckState(0, Qt::Unchecked);
                leaf->setData(0, Qt::UserRole, id);
                leaf->setData(0, Qt::UserRole + 1, i);
                QIcon leafIcon = (e.type == 1) ? QIcon::fromTheme(e.cmd) : QIcon();
                if (leafIcon.isNull()) leafIcon = QIcon::fromTheme("text-x-hex");
                leaf->setIcon(0, leafIcon);
            }
            profileItems.insert(id, top);
        }
        updatingTree = false;

        setProfileChecked("essential", true);
    }

    void onTreeItemChanged(QTreeWidgetItem* item, int column) {
        if (updatingTree || column != 0) return;

        if (item->parent() == nullptr) {
            setProfileChecked(item->data(0, Qt::UserRole).toString(),
                              item->checkState(0) == Qt::Checked);
            return;
        }

        QTreeWidgetItem* parent = item->parent();
        int checkedCount = 0;
        for (int i = 0; i < parent->childCount(); ++i) {
            if (parent->child(i)->checkState(0) == Qt::Checked) ++checkedCount;
        }

        updatingTree = true;
        if (checkedCount == 0)
            parent->setCheckState(0, Qt::Unchecked);
        else if (checkedCount == parent->childCount())
            parent->setCheckState(0, Qt::Checked);
        else
            parent->setCheckState(0, Qt::PartiallyChecked);
        updatingTree = false;

        const QString id = parent->data(0, Qt::UserRole).toString();
        if (QPushButton* btn = suiteButtons.value(id)) {
            QSignalBlocker block(btn);
            btn->setChecked(checkedCount > 0);
        }

        if (id == "minimal") applyMinimalExclusivity(checkedCount > 0);

        updateCommand();
        refreshNextLabel();
    }

    void setProfileChecked(const QString& id, bool checked) {
        QTreeWidgetItem* top = profileItems.value(id, nullptr);
        if (!top) return;

        updatingTree = true;
        top->setCheckState(0, checked ? Qt::Checked : Qt::Unchecked);
        for (int i = 0; i < top->childCount(); ++i) {
            top->child(i)->setCheckState(0, checked ? Qt::Checked : Qt::Unchecked);
        }
        updatingTree = false;

        if (QPushButton* btn = suiteButtons.value(id)) {
            QSignalBlocker block(btn);
            btn->setChecked(checked);
        }

        if (id == "minimal") applyMinimalExclusivity(checked);

        updateCommand();
        refreshNextLabel();
    }

    void applyMinimalExclusivity(bool minimalChecked) {
        for (auto it = suiteButtons.begin(); it != suiteButtons.end(); ++it) {
            if (it.key() == "minimal") continue;
            it.value()->setEnabled(!minimalChecked);
        }
        for (auto it = profileItems.begin(); it != profileItems.end(); ++it) {
            if (it.key() == "minimal") continue;
            it.value()->setDisabled(minimalChecked);
            if (minimalChecked) {
                updatingTree = true;
                it.value()->setCheckState(0, Qt::Unchecked);
                for (int i = 0; i < it.value()->childCount(); ++i)
                    it.value()->child(i)->setCheckState(0, Qt::Unchecked);
                updatingTree = false;
                if (QPushButton* btn = suiteButtons.value(it.key())) {
                    QSignalBlocker block(btn);
                    btn->setChecked(false);
                }
            }
        }
    }

    bool anythingSelected() const {
        for (auto it = profileItems.constBegin(); it != profileItems.constEnd(); ++it) {
            QTreeWidgetItem* top = it.value();
            if (top->checkState(0) != Qt::Unchecked) return true;
        }
        return false;
    }
    void refreshNextLabel() {
        if (step != 3) return;
        Requested = anythingSelected();
        nextBtn->setText(Requested ? "Install & Next" : "Continue");
    }

    void showStep(int s) {
        step = s;
        stack->setCurrentIndex(s);

        progressAnim->stop();
        progressAnim->setStartValue(progress->value());
        progressAnim->setEndValue((s + 1) * 20);
        progressAnim->start();

        backBtn->setVisible(s > 0);
        backBtn->setEnabled(s > 0);

        if (s == 3) {
            updateCommand();
            refreshNextLabel();
        } else if (s == 4) {
            nextBtn->setText("Finish");
        } else {
            nextBtn->setText("Next");
        }

        nextBtn->setEnabled(s == 0 || (s == 1 && winKeyPressed) || (s == 2 && licenseOK) || s == 3 || s == 4);
    }

    void updateCommand() {
        QStringList aptPkgs;
        QStringList scripts;

        for (auto it = profileItems.constBegin(); it != profileItems.constEnd(); ++it) {
            const QString id = it.key();
            if (!profiles.contains(id)) continue;
            QTreeWidgetItem* top = it.value();
            for (int i = 0; i < top->childCount(); ++i) {
                QTreeWidgetItem* leaf = top->child(i);
                if (leaf->checkState(0) != Qt::Checked) continue;
                const int idx = leaf->data(0, Qt::UserRole + 1).toInt();
                if (idx < 0 || idx >= profiles[id].packages.size()) continue;
                const PkgEntry& e = profiles[id].packages[idx];
                if (e.type == 1) aptPkgs << e.cmd;
                else             scripts << e.cmd;
            }
        }

        aptPkgs.removeDuplicates();
        aptPkgs.sort();
        scripts.removeDuplicates();

        if (aptPkgs.isEmpty() && scripts.isEmpty()) {
            targetCmd = "Select profiles, or skip it.";
        } else {
            QStringList parts;
            if (!aptPkgs.isEmpty())
                parts << "sudo apt update && sudo apt install -y " + aptPkgs.join(" ");
            targetCmd = parts.join(" && ");
            for (const QString& s : scripts) {
                if (targetCmd.isEmpty())
                    targetCmd = s;
                else
                    targetCmd += "; " + s;
            }
        }

        cmdView->clearTerminal();
        cmdView->typeText(targetCmd);
    }

    void handleNext() {
        if (step == 3) {
            QString cmd = targetCmd;
            if (!cmd.isEmpty() && cmd != "Select profiles to see installation command"
                && anythingSelected()) {

                QString script = QStringLiteral(R"BASH(
LOG=/tmp/error.os-setup-$(date +%s).log
clear
echo "Executing:-"
echo
echo "    %1"
echo
read -p "Press Enter to continue..."
{
  echo "once (error.os) setup logs."
  date
  echo
  %1
  RC=$?
  echo
  echo "exit: $RC"
} 2>&1 | tee "$LOG"
if [ $RC -ne 0 ]; then
  cp "$LOG" "$HOME/error_in_setup.log"
  echo
  echo "Command failed. Log saved to ~/error_in_setup.log"
fi
echo
read -p "Press Enter to close..."
)BASH").arg(cmd);

                QStringList terminals = {"konsole", "xterm"};
                for (const QString& term : terminals) {
                    if (QProcess::startDetached(term, {"-e", "bash", "-c", script})) {
                        break;
                    }
                }
            }
            step++;
            showStep(step);
        } else if (step < 4) {
            step++;
            showStep(step);
        } else {
            QString configPath = QStandardPaths::writableLocation(QStandardPaths::ConfigLocation);
            QString autostartDir = configPath + "/autostart";
            QDir().mkpath(autostartDir);
            QString autostartFile = autostartDir + "/once.desktop";
            QFile::remove(autostartFile);
            qApp->quit();
        }
    }

    void handleBack() {
        if (step > 0) {
            step--;
            showStep(step);
        }
    }

    void handleClose() {
        QMessageBox::StandardButton reply = QMessageBox::question(
            this,
            "Close Setup?",
            "Are you sure you want to close the setup?\n"
            "The setup will appear again on next startup.",
            QMessageBox::Yes | QMessageBox::No,
            QMessageBox::No
            );
        if (reply == QMessageBox::Yes) {
            qApp->quit();
        }
    }
};

#include "main.moc"

int main(int argc, char** argv) {
    QApplication app(argc, argv);
    app.setStyleSheet(QSS);
    OnboardingTour tour;
    tour.showFullScreen();
    return app.exec();
}
