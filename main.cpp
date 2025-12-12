#include <QApplication>
#include <QWidget>
#include <QVBoxLayout>
#include <QHBoxLayout>
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

class OnboardingTour;
class LicenseViewer;

class LicenseViewer : public QMainWindow {
    Q_OBJECT
public:
    explicit LicenseViewer(const QString& licenseText, QWidget* parent = nullptr)
        : QMainWindow(parent), fullText(licenseText) {
        setWindowTitle("License Agreement");
        resize(760, 520);
        setWindowModality(Qt::ApplicationModal);
        setWindowFlags(windowFlags() & ~Qt::WindowMinimizeButtonHint & ~Qt::WindowCloseButtonHint);
        auto* central = new QWidget(this);
        auto* layout = new QVBoxLayout(central);

        auto* scrollArea = new QScrollArea(this);
        scrollArea->setWidgetResizable(true);
        scrollArea->setStyleSheet("QScrollArea { border: 1px solid #333; background: #0a0a12; }");

        textEdit = new QTextEdit(this);
        textEdit->setReadOnly(true);
        textEdit->setStyleSheet(
            "QTextEdit {"
            "  background-color: #0a0a12;"
            "  color: #00ff80;"
            "  font-family: monospace;"
            "  font-size: 13px;"
            "  border: none;"
            "  padding: 12px;"
            "}"
            );

        scrollArea->setWidget(textEdit);

        auto* btnLayout = new QHBoxLayout();

        auto* copyBtn = new QPushButton("Copy to Clipboard", this);
        copyBtn->setStyleSheet("padding: 8px 16px; font-size: 12px;");
        connect(copyBtn, &QPushButton::clicked, this, [this]() {
            QGuiApplication::clipboard()->setText(fullText);
        });

        okBtn = new QPushButton("OK", this);
        okBtn->setStyleSheet("padding: 8px 24px; font-size: 14px; font-weight: bold; background: #5a6fff; color: white;");
        okBtn->setVisible(false);
        connect(okBtn, &QPushButton::clicked, this, &LicenseViewer::close);

        btnLayout->addWidget(copyBtn);
        btnLayout->addStretch();
        btnLayout->addWidget(okBtn);

        layout->addWidget(new QLabel("<h3 style='color: #e0e0ff; text-align: center;'>Terms of error</h3>"));
        layout->addWidget(scrollArea, 1);
        layout->addLayout(btnLayout);

        setCentralWidget(central);

        connect(&typeTimer, &QTimer::timeout, this, &LicenseViewer::typeNextChar);

        typeTimer.setInterval(60);
        currentIndex = 0;
        typeTimer.start();
    }

signals:
    void finished();

protected:
    void closeEvent(QCloseEvent* event) override {
        typeTimer.stop();
        scrollTimer.stop();
        emit finished();
        event->accept();
    }




private slots:
    void typeNextChar() {
        if (currentIndex < fullText.length()) {
            float progress = (float)currentIndex / fullText.length();
            int interval;
            int charsPerTick;

            if (progress < 0.1) {
                interval = 50;
                charsPerTick = 2;
            } else if (progress < 0.3) {
                interval = 30;
                charsPerTick = 5;
            } else if (progress < 0.6) {
                interval = 10;
                charsPerTick = 15;
            } else {
                interval = 1;
                charsPerTick = 30;
            }

            QTextCursor cursor = textEdit->textCursor();
            cursor.movePosition(QTextCursor::End);
            cursor.insertText(fullText.mid(currentIndex, charsPerTick));
            textEdit->setTextCursor(cursor);

            currentIndex += charsPerTick;
            typeTimer.setInterval(interval);
        } else {
            typeTimer.stop();
            okBtn->setVisible(true);
        }
    }

private:
    QTextEdit* textEdit = nullptr;
    QPushButton* okBtn = nullptr;
    QTimer typeTimer;
    QTimer scrollTimer;
    QString fullText;
    int currentIndex = 0;
};

class OnboardingTour : public QWidget {
    Q_OBJECT

    struct Profile {
        QString name;
        QString icon;
        QString desc;
        QStringList apps;
    };

    QStackedWidget* stack;
    QPushButton *nextBtn, *backBtn;
    QProgressBar* progress;
    QMap<QString, Profile> profiles;
    QSet<QString> selected;
    QMap<QString, QWidget*> appLists;
    QTextEdit* cmdView;
    int step = 0;
    bool winKeyPressed = false;
    bool licenseOK = false;

    QTimer* cmdTypewriter;
    QString targetCmd;
    int cmdIndex = 0;

public:
    OnboardingTour(QWidget* parent = nullptr) : QWidget(parent) {
        setWindowFlags(Qt::FramelessWindowHint);
        setAttribute(Qt::WA_TranslucentBackground);
        setWindowIcon(QIcon::fromTheme("system-software-install"));
        QScreen* screen = QGuiApplication::primaryScreen();
        setGeometry(screen->geometry());

        setupProfiles();
        setupUI();
        installEventFilter(this);
        showStep(0);

        cmdTypewriter = new QTimer(this);
        connect(cmdTypewriter, &QTimer::timeout, this, &OnboardingTour::typeNextCmdChar);
    }

    void paintEvent(QPaintEvent*) override {
        QPainter p(this);
        p.setRenderHint(QPainter::Antialiasing);

        int margin = 20;
        QRect inner = rect().adjusted(margin, margin, -margin, -margin);

        QPainterPath path;
        path.addRoundedRect(inner, 20, 20);

        p.fillPath(path, QColor(0, 0, 0, 245));
        setMask(QRegion(path.toFillPolygon().toPolygon()));
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
        profiles["minimal"] = {"Minimal", "edit-delete", "Stripped down current system dont choose other options if you have very low storage only choose this",
                               {"nnn && sudo apt remove --allow-remove-essentials konsole mpv featherpad plasma-discover plasma-discover-backend-fwupd kde-spectacle kdeconnect plasma-firewall pipewire-pulse plasma-workspace qt6-style-kvantum dolphin && sudo apt autoremove && sudo apt install --no-install-recommends zutty"}};

        profiles["essential"] = {"Essential", "applications-internet", "Browser, media player and image viewer - Daily use basics",
                                 {"falkon", "mpv", "qimgv", "qmmp"}};

        profiles["gaming"] = {"Gaming", "applications-games", "Native Linux games - Not recommended for low-end devices",
                              {"lutris", "xonotic", "teeworlds", "supertux", "supertuxkart"}};

        profiles["dev"] = {"Developer", "applications-development", "Programming tools - 900+ MB download",
                           {" git make && wget -O vscode.deb 'https://code.visualstudio.com/sha/download?build=stable&os=linux-deb-x64' && sudo dpkg -i vscode.deb || sudo apt install -f && rm vscode.deb && sudo apt install -y  gcc"}};

        profiles["art"] = {"Digital Art", "lazpaint", "200+ MB download",
                           {"inkscape", "krita", "scribus" }};

        profiles["designer"] = {"Graphic Designer", "applications-graphics", "Photo, video and 3D editing - 500+ MB download",
                                {"gimp", "inkscape", "kdenlive", "blender"}};

        profiles["server"] = {"File Server", "network-server", "Samba file sharing setup - 200+ MB download",
                              { "samba samba-common-bin kdenetwork-filesharing dolphin-plugins smb4k "}};

        profiles["student"] = {"Student", "applications-education", "Study tools and productivity apps - 1.2+ GB download",
                               {"libreoffice", "chromium", "okular", "octave", "anki", "thunderbird", "vlc", "obs-studio", "kalzium", "kstars"}};

        profiles["cyber"] = {"Cyber security", "nethack", "For security experts hackers and exploiters",
                               {"nmap", "wireshark", "john", "hydra", "sqlmap", "metasploit-framework", "burpsuite", "aircrack-ng", "hashcat", "gobuster"}};

    }

    void setupUI() {
        QVBoxLayout* main = new QVBoxLayout(this);
        main->setContentsMargins(40, 40, 40, 40);

        QHBoxLayout* header = new QHBoxLayout();
        QLabel* title = new QLabel("error.os Setup");
        title->setStyleSheet("font-size: 20px; font-weight: bold; color: #d0d0ff;");
        QPushButton* close = new QPushButton(QIcon::fromTheme("window-close"), "");
        close->setFixedSize(40, 40);
        close->setStyleSheet("QPushButton { background: transparent; color: #ff5555; font-size: 20px; border: none; }"
                             "QPushButton:hover { background: rgba(255,85,85,100); border-radius: 20px; }");
        connect(close, &QPushButton::clicked, this, &OnboardingTour::handleClose);
        header->addWidget(title);
        header->addStretch();
        header->addWidget(close);

        progress = new QProgressBar();
        progress->setRange(0, 100);
        progress->setTextVisible(false);
        progress->setFixedHeight(4);
        progress->setStyleSheet("QProgressBar { background: #252535; border: none; border-radius: 2px; }"
                                "QProgressBar::chunk { background: qlineargradient(x1:0, x2:1, stop:0 #00bfff, stop:1 #7a7fff); }");

        stack = new QStackedWidget();
        createSteps();

        QHBoxLayout* footer = new QHBoxLayout();
        backBtn = new QPushButton("Back");
        nextBtn = new QPushButton("Next");
        backBtn->setMinimumSize(120, 45);
        nextBtn->setMinimumSize(140, 45);
        backBtn->setStyleSheet("QPushButton { background: #2f2f42; color: #c0c0e0; border-radius: 8px; font-size: 15px; }"
                               "QPushButton:disabled { background: #1a1a24; color: #555; }");
        nextBtn->setStyleSheet("QPushButton { background: qlineargradient(y1:0, y2:1, stop:0 #6c7fff, stop:1 #5a6fff); "
                               "color: white; border-radius: 8px; font-size: 15px; font-weight: bold; }"
                               "QPushButton:disabled { background: #3a3a4a; color: #888; }");
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
        t0->setStyleSheet("font-size: 42px; font-weight: bold; color: #e0e0ff;");
        QLabel* e0 = new QLabel("Neospace  2025");
        e0->setStyleSheet("font-size: 38px; font-weight: bold; color: #e0e0ff;");
        e0->setAlignment(Qt::AlignCenter);
        QLabel* d0 = new QLabel("Let's set up your system in a few simple steps");
        d0->setStyleSheet("font-size: 18px; color: #b0b0d0;");
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
        t1->setStyleSheet("font-size: 38px; font-weight: bold; color: #e0e0ff;");
        t1->setAlignment(Qt::AlignCenter);
        QLabel* d1 = new QLabel("This is how you open your application launcher\n(This key is also called Meta or Super key in Linux)");
        QLabel* w2 = new QLabel("here are few shortcuts if you seek to know,\n meta+w - meta+d - meta+e .. check settings for more");
        d1->setStyleSheet("font-size: 16px; color: #b0b0d0;");
        d1->setAlignment(Qt::AlignCenter);
        w2->setStyleSheet("font-size: 10px; color: #b0b0d0;");
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
        t2->setStyleSheet("font-size: 34px; font-weight: bold; color: #e0e0ff;");
        t2->setAlignment(Qt::AlignCenter);
        QLabel* d2 = new QLabel("Review to go ahead");
        d2->setStyleSheet("font-size: 16px; color: #b0b0d0;");
        d2->setAlignment(Qt::AlignCenter);

        QString licenseText = R"(WELCOME TO error.os
Consider this as a fun journey
This isn't just another boring linux operating system,

• TRUE OWNERSHIP: This computer belongs to you now. Customize everything!
• EMBRACE THE GLITCHES: Things will break sometimes. That's where the real learning happens.
• FOUND A BUG? report in the github page https://github.com/zynomon/error or
   try fixing yourself that is what embracing error truly means
• MAKE IT BEAUTIFUL: Theme it, tweak it, make it unrecognizable. Go crazy!
• EXPERIMENT FEARLESSLY: Break things! Discover what's possible through trial and error.
• SHARE YOUR CREATIONS: Show off your setups. Inspire others!
• BACKUP YOUR WORK: Protect your important files. Always.
• MOST IMPORTANTLY: Enjoy the journey! Have fun exploring.
• TOTAL PRIVACY: We don't track you. No telemetry, no data collection, not even oour personal website with a paid domain.

                  Ready to create something amazing? Let's begin!
                 for help and other stuffs join our discord  server
––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––


                                 Apache License
                           Version 2.0, January 2004
                        http://www.apache.org/licenses/

   TERMS AND CONDITIONS FOR USE, REPRODUCTION, AND DISTRIBUTION

   1. Definitions.

      "License" shall mean the terms and conditions for use, reproduction,
      and distribution as defined by Sections 1 through 9 of this document.

      "Licensor" shall mean the copyright owner or entity authorized by
      the copyright owner that is granting the License.

      "Legal Entity" shall mean the union of the acting entity and all
      other entities that control, are controlled by, or are under common
      control with that entity. For the purposes of this definition,
      "control" means (i) the power, direct or indirect, to cause the
      direction or management of such entity, whether by contract or
      otherwise, or (ii) ownership of fifty percent (50%) or more of the
      outstanding shares, or (iii) beneficial ownership of such entity.

      "You" (or "Your") shall mean an individual or Legal Entity
      exercising permissions granted by this License.

      "Source" form shall mean the preferred form for making modifications,
      including but not limited to software source code, documentation
      source, and configuration files.

      "Object" form shall mean any form resulting from mechanical
      transformation or translation of a Source form, including but
      not limited to compiled object code, generated documentation,
      and conversions to other media types.

      "Work" shall mean the work of authorship, whether in Source or
      Object form, made available under the License, as indicated by a
      copyright notice that is included in or attached to the work
      (an example is provided in the Appendix below).

      "Derivative Works" shall mean any work, whether in Source or Object
      form, that is based on (or derived from) the Work and for which the
      editorial revisions, annotations, elaborations, or other modifications
      represent, as a whole, an original work of authorship. For the purposes
      of this License, Derivative Works shall not include works that remain
      separable from, or merely link (or bind by name) to the interfaces of,
      the Work and Derivative Works thereof.

      "Contribution" shall mean any work of authorship, including
      the original version of the Work and any modifications or additions
      to that Work or Derivative Works thereof, that is intentionally
      submitted to Licensor for inclusion in the Work by the copyright owner
      or by an individual or Legal Entity authorized to submit on behalf of
      the copyright owner. For the purposes of this definition, "submitted"
      means any form of electronic, verbal, or written communication sent
      to the Licensor or its representatives, including but not limited to
      communication on electronic mailing lists, source code control systems,
      and issue tracking systems that are managed by, or on behalf of, the
      Licensor for the purpose of discussing and improving the Work, but
      excluding communication that is conspicuously marked or otherwise
      designated in writing by the copyright owner as "Not a Contribution."

      "Contributor" shall mean Licensor and any individual or Legal Entity
      on behalf of whom a Contribution has been received by Licensor and
      subsequently incorporated within the Work.

   2. Grant of Copyright License. Subject to the terms and conditions of
      this License, each Contributor hereby grants to You a perpetual,
      worldwide, non-exclusive, no-charge, royalty-free, irrevocable
      copyright license to reproduce, prepare Derivative Works of,
      publicly display, publicly perform, sublicense, and distribute the
      Work and such Derivative Works in Source or Object form.

   3. Grant of Patent License. Subject to the terms and conditions of
      this License, each Contributor hereby grants to You a perpetual,
      worldwide, non-exclusive, no-charge, royalty-free, irrevocable
      (except as stated in this section) patent license to make, have made,
      use, offer to sell, sell, import, and otherwise transfer the Work,
      where such license applies only to those patent claims licensable
      by such Contributor that are necessarily infringed by their
      Contribution(s) alone or by combination of their Contribution(s)
      with the Work to which such Contribution(s) was submitted. If You
      institute patent litigation against any entity (including a
      cross-claim or counterclaim in a lawsuit) alleging that the Work
      or a Contribution incorporated within the Work constitutes direct
      or contributory patent infringement, then any patent licenses
      granted to You under this License for that Work shall terminate
      as of the date such litigation is filed.

   4. Redistribution. You may reproduce and distribute copies of the
      Work or Derivative Works thereof in any medium, with or without
      modifications, and in Source or Object form, provided that You
      meet the following conditions:

      (a) You must give any other recipients of the Work or
          Derivative Works a copy of this License; and

      (b) You must cause any modified files to carry prominent notices
          stating that You changed the files; and

      (c) You must retain, in the Source form of any Derivative Works
          that You distribute, all copyright, patent, trademark, and
          attribution notices from the Source form of the Work,
          excluding those notices that do not pertain to any part of
          the Derivative Works; and

      (d) If the Work includes a "NOTICE" text file as part of its
          distribution, then any Derivative Works that You distribute must
          include a readable copy of the attribution notices contained
          within such NOTICE file, excluding those notices that do not
          pertain to any part of the Derivative Works, in at least one
          of the following places: within a NOTICE text file distributed
          as part of the Derivative Works; within the Source form or
          documentation, if provided along with the Derivative Works; or,
          within a display generated by the Derivative Works, if and
          wherever such third-party notices normally appear. The contents
          of the NOTICE file are for informational purposes only and
          do not modify the License. You may add Your own attribution
          notices within Derivative Works that You distribute, alongside
          or as an addendum to the NOTICE text from the Work, provided
          that such additional attribution notices cannot be construed
          as modifying the License.

      You may add Your own copyright statement to Your modifications and
      may provide additional or different license terms and conditions
      for use, reproduction, or distribution of Your modifications, or
      for any such Derivative Works as a whole, provided Your use,
      reproduction, and distribution of the Work otherwise complies with
      the conditions stated in this License.

   5. Submission of Contributions. Unless You explicitly state otherwise,
      any Contribution intentionally submitted for inclusion in the Work
      by You to the Licensor shall be under the terms and conditions of
      this License, without any additional terms or conditions.
      Notwithstanding the above, nothing herein shall supersede or modify
      the terms of any separate license agreement you may have executed
      with Licensor regarding such Contributions.

   6. Trademarks. This License does not grant permission to use the trade
      names, trademarks, service marks, or product names of the Licensor,
      except as required for reasonable and customary use in describing the
      origin of the Work and reproducing the content of the NOTICE file.

   7. Disclaimer of Warranty. Unless required by applicable law or
      agreed to in writing, Licensor provides the Work (and each
      Contributor provides its Contributions) on an "AS IS" BASIS,
      WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or
      implied, including, without limitation, any warranties or conditions
      of TITLE, NON-INFRINGEMENT, MERCHANTABILITY, or FITNESS FOR A
      PARTICULAR PURPOSE. You are solely responsible for determining the
      appropriateness of using or redistributing the Work and assume any
      risks associated with Your exercise of permissions under this License.

   8. Limitation of Liability. In no event and under no legal theory,
      whether in tort (including negligence), contract, or otherwise,
      unless required by applicable law (such as deliberate and grossly
      negligent acts) or agreed to in writing, shall any Contributor be
      liable to You for damages, including any direct, indirect, special,
      incidental, or consequential damages of any character arising as a
      result of this License or out of the use or inability to use the
      Work (including but not limited to damages for loss of goodwill,
      work stoppage, computer failure or malfunction, or any and all
      other commercial damages or losses), even if such Contributor
      has been advised of the possibility of such damages.

   9. Accepting Warranty or Additional Liability. While redistributing
      the Work or Derivative Works thereof, You may choose to offer,
      and charge a fee for, acceptance of support, warranty, indemnity,
      or other liability obligations and/or rights consistent with this
      License. However, in accepting such obligations, You may act only
      on Your own behalf and on Your sole responsibility, not on behalf
      of any other Contributor, and only if You agree to indemnify,
      defend, and hold each Contributor harmless for any liability
      incurred by, or claims asserted against, such Contributor by reason
      of your accepting any such warranty or additional liability.

   END OF TERMS AND CONDITIONS

   APPENDIX: How to apply the Apache License to your work.

      To apply the Apache License to your work, attach the following
      boilerplate notice, with the fields enclosed by brackets "[]"
      replaced with your own identifying information. (Don't include
      the brackets!)  The text should be enclosed in the appropriate
      comment syntax for the file format. We also recommend that a
      file or class name and description of purpose be included on the
      same "printed page" as the copyright notice for easier
      identification within third-party archives.

   Copyright 2025 error.os  - ZYNOMON AELIUS
   Licensed under the Apache License, Version 2.0 (the "License");
   you may not use this file except in compliance with the License.
   You may obtain a copy of the License at

       http://www.apache.org/licenses/LICENSE-2.0

   Unless required by applicable law or agreed to in writing, software
   distributed under the License is distributed on an "AS IS" BASIS,
   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
   See the License for the specific language governing permissions and
   limitations under the License.

––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––

                          Thanks for your patients 😅

)";

        QPushButton* viewLicense = new QPushButton("View License");
        viewLicense->setMinimumSize(200, 50);
        viewLicense->setStyleSheet("background: #5a6fff; color: white; border-radius: 10px; font-size: 16px; font-weight: bold;");
        connect(viewLicense, &QPushButton::clicked, [this, licenseText]() {
            LicenseViewer* viewer = new LicenseViewer(licenseText, this);
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
        t3->setStyleSheet("font-size: 28px; font-weight: bold; color: #e0e0ff;");
        l3->addWidget(t3);

        QScrollArea* scroll = new QScrollArea();
        scroll->setWidgetResizable(true);
        scroll->setStyleSheet("QScrollArea { border: none; background: transparent; }");

        QWidget* scrollContent = new QWidget();
        QVBoxLayout* scrollLayout = new QVBoxLayout(scrollContent);
        scrollLayout->setSpacing(10);

        for (auto it = profiles.begin(); it != profiles.end(); ++it) {
            QString id = it.key();
            Profile& p = it.value();

            QWidget* item = new QWidget();
            item->setStyleSheet("background: rgba(40,40,60,200); border-radius: 10px; padding: 12px;");
            QVBoxLayout* itemLayout = new QVBoxLayout(item);
            itemLayout->setSpacing(8);

            QHBoxLayout* headerLayout = new QHBoxLayout();
            QLabel* iconLabel = new QLabel();
            iconLabel->setPixmap(QIcon::fromTheme(p.icon).pixmap(24, 24));
            QCheckBox* cb = new QCheckBox(p.name + " - " + p.desc);
            cb->setStyleSheet("font-size: 15px; color: white; font-weight: bold;");
            if (id == "essential") cb->setChecked(true);

            connect(cb, &QCheckBox::toggled, [this, id, cb]() {
                if (cb->isChecked()) selected.insert(id);
                else selected.remove(id);
                updateCommand();
                if (appLists.contains(id)) {
                    appLists[id]->setVisible(cb->isChecked());
                }
            });

            headerLayout->addWidget(iconLabel);
            headerLayout->addWidget(cb, 1);
            itemLayout->addLayout(headerLayout);

            QWidget* appList = new QWidget();
            appList->setStyleSheet("background: rgba(20,20,30,180); border-radius: 6px; padding: 10px; margin-left: 30px;");
            QVBoxLayout* appLayout = new QVBoxLayout(appList);
            appLayout->setSpacing(4);

            QString appText;
            for (const QString& app : p.apps) {
                appText += "• " + app + "\n";
            }
            QLabel* apps = new QLabel(appText);
            apps->setWordWrap(true);
            apps->setStyleSheet("color: #a0a0c0; font-size: 12px;");
            appLayout->addWidget(apps);

            appList->setVisible(id == "essential");
            appLists[id] = appList;
            itemLayout->addWidget(appList);

            scrollLayout->addWidget(item);

            if (id == "essential") selected.insert(id);
        }

        scroll->setWidget(scrollContent);
        l3->addWidget(scroll);

        cmdView = new QTextEdit();
        cmdView->setReadOnly(true);
        cmdView->setMaximumHeight(100);
        cmdView->setStyleSheet("background: #000; color: #00ff80; font-family: monospace; font-size: 13px; border: 1px solid #333; padding: 8px;");
        l3->addWidget(cmdView);

        stack->addWidget(s3);

        QWidget* s4 = new QWidget();
        QVBoxLayout* l4 = new QVBoxLayout(s4);
        l4->setAlignment(Qt::AlignCenter);
        l4->setSpacing(30);
        QLabel* icon4 = new QLabel();
        icon4->setPixmap(QIcon::fromTheme("dialog-ok").pixmap(96, 96));
        icon4->setAlignment(Qt::AlignCenter);
        QLabel* t4 = new QLabel("All Set!");
        t4->setStyleSheet("font-size: 42px; font-weight: bold; color: #e0e0ff;");
        t4->setAlignment(Qt::AlignCenter);
        QLabel* d4 = new QLabel("Setup is complete! A TERMINAL will open to install the packages, give it your password - that you used to login and Thats all enjoy error.os :)");
        d4->setStyleSheet("font-size: 16px; color: #b0b0d0;");
        d4->setAlignment(Qt::AlignCenter);
        d4->setWordWrap(true);
        l4->addWidget(icon4);
        l4->addWidget(t4);
        l4->addWidget(d4);
        stack->addWidget(s4);
    }

    void showStep(int s) {
        step = s;
        stack->setCurrentIndex(s);
        progress->setValue((s + 1) * 20);

        backBtn->setVisible(s > 0);
        backBtn->setEnabled(s > 0);

        if (s == 3) {
            nextBtn->setText("Install and next");
            nextBtn->setStyleSheet("QPushButton { background: #00cc66; color: white; border-radius: 8px; font-size: 14px; font-weight: bold; }"
                                   "QPushButton:disabled { background: #3a3a4a; color: #888; }");
            updateCommand();
        } else if (s == 4) {
            nextBtn->setText("Finish");
            nextBtn->setStyleSheet("QPushButton { background: qlineargradient(y1:0, y2:1, stop:0 #6c7fff, stop:1 #5a6fff); "
                                   "color: white; border-radius: 8px; font-size: 15px; font-weight: bold; }"
                                   "QPushButton:disabled { background: #3a3a4a; color: #888; }");
        } else {
            nextBtn->setText("Next");
            nextBtn->setStyleSheet("QPushButton { background: qlineargradient(y1:0, y2:1, stop:0 #6c7fff, stop:1 #5a6fff); "
                                   "color: white; border-radius: 8px; font-size: 15px; font-weight: bold; }"
                                   "QPushButton:disabled { background: #3a3a4a; color: #888; }");
        }

        nextBtn->setEnabled(s == 0 || (s == 1 && winKeyPressed) || (s == 2 && licenseOK) || s == 3 || s == 4);
    }

    void updateCommand() {
        QStringList allApps;
        for (const QString& id : selected) {
            allApps.append(profiles[id].apps);
        }

        if (allApps.isEmpty()) {
            targetCmd = "Select profiles to see installation command";
        } else {
            allApps.removeDuplicates();
            allApps.sort();
            targetCmd = "sudo apt update && sudo apt install -y " + allApps.join(" ");
        }

        cmdTypewriter->stop();
        cmdIndex = 0;
        cmdView->clear();
        cmdTypewriter->start(5);
    }

    void typeNextCmdChar() {
        if (cmdIndex < targetCmd.length()) {
            cmdView->insertPlainText(targetCmd.mid(cmdIndex, 1));
            ++cmdIndex;
            QTextCursor c = cmdView->textCursor();
            c.movePosition(QTextCursor::End);
            cmdView->setTextCursor(c);
        } else {
            cmdTypewriter->stop();
        }
    }

    void handleNext() {
        if (step == 3) {
            QString cmd = cmdView->toPlainText();
            if (!cmd.isEmpty() && cmd != "Select profiles to see installation command") {
                QStringList terminals = {"konsole", "gnome-terminal", "xterm", "alacritty"};
                for (const QString& term : terminals) {
                    if (QProcess::startDetached(term, {"-e", "bash", "-c", cmd + "; read -p 'Press Enter to close...'"})) {
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
    OnboardingTour tour;
    tour.show();
    return app.exec();
}
