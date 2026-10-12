// Copyright (C) 2024-2026 UnionTech Software Technology Co., Ltd.
// SPDX-License-Identifier: Apache-2.0 OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "modules/shortcut/shortcutcontroller.h"
#include "modules/shortcut/shortcutmanager.h"

#include <wserver.h>

#include <QKeyEvent>
#include <QObject>
#include <QTest>

class ShortcutTest : public QObject
{
    Q_OBJECT

    WAYLIB_SERVER_NAMESPACE::WServer *m_server = nullptr;

public:
    ShortcutTest(QObject *parent = nullptr)
        : QObject(parent)
    {
    }

private Q_SLOTS:

    void initTestCase()
    {
        m_server = new WAYLIB_SERVER_NAMESPACE::WServer();
    }

    void testCreate()
    {
        QVERIFY(m_server->attach<ShortcutManagerV2>(m_server) != nullptr);
    }

    void verifyShortcut()
    {
        auto protocol = m_server->findChild<ShortcutManagerV2*>();
        QVERIFY(protocol != nullptr);
    }

    void testModifierOnly()
    {
        ShortcutController controller;
        int triggered = 0;
        connect(&controller, &ShortcutController::actionTriggered, this,
                [&triggered](ShortcutAction, const QString &, bool, ShortcutController::KeyFlags) {
            ++triggered;
        });

        QCOMPARE(controller.registerKey(QStringLiteral("meta-action"), QStringLiteral("Meta"),
                                        ShortcutController::KeyRelease, ShortcutAction::Notify),
                 0u);

        // Pressing Meta alone must not fire the shortcut: the client still needs
        // to see the key press.
        QKeyEvent press(QEvent::KeyPress, Qt::Key_Meta, Qt::MetaModifier);
        QVERIFY(!controller.dispatchKeyEvent(&press));
        QCOMPARE(triggered, 0);

        // Releasing Meta completes the modifier-only shortcut. The release is
        // still not consumed: the client must receive it.
        QKeyEvent release(QEvent::KeyRelease, Qt::Key_Meta, Qt::NoModifier);
        QVERIFY(!controller.dispatchKeyEvent(&release));
        QCOMPARE(triggered, 1);
    }

    void testModifierOnlyCancelledByRegularKey()
    {
        ShortcutController controller;
        int triggered = 0;
        connect(&controller, &ShortcutController::actionTriggered, this,
                [&triggered](ShortcutAction, const QString &, bool, ShortcutController::KeyFlags) {
            ++triggered;
        });

        QCOMPARE(controller.registerKey(QStringLiteral("meta-action"), QStringLiteral("Meta"),
                                        ShortcutController::KeyRelease, ShortcutAction::Notify),
                 0u);

        // Super+Tab where Tab is not a shortcut: all four events pass through
        // unchanged, and the standalone Meta action must not fire.
        QKeyEvent metaPress(QEvent::KeyPress, Qt::Key_Meta, Qt::MetaModifier);
        QVERIFY(!controller.dispatchKeyEvent(&metaPress));

        QKeyEvent tabPress(QEvent::KeyPress, Qt::Key_Tab, Qt::MetaModifier);
        QVERIFY(!controller.dispatchKeyEvent(&tabPress));

        QKeyEvent tabRelease(QEvent::KeyRelease, Qt::Key_Tab, Qt::MetaModifier);
        QVERIFY(!controller.dispatchKeyEvent(&tabRelease));

        QKeyEvent metaRelease(QEvent::KeyRelease, Qt::Key_Meta, Qt::NoModifier);
        QVERIFY(!controller.dispatchKeyEvent(&metaRelease));

        QCOMPARE(triggered, 0);
    }

    void testRegularShortcutModifiersAreNotConsumed()
    {
        ShortcutController controller;
        int triggered = 0;
        connect(&controller, &ShortcutController::actionTriggered, this,
                [&triggered](ShortcutAction, const QString &, bool, ShortcutController::KeyFlags) {
            ++triggered;
        });

        QCOMPARE(controller.registerKey(QStringLiteral("ctrl-k"), QStringLiteral("Ctrl+K"),
                                        ShortcutController::KeyPress, ShortcutAction::Notify),
                 0u);

        // Ctrl press/release are never consumed.
        QKeyEvent ctrlPress(QEvent::KeyPress, Qt::Key_Control, Qt::ControlModifier);
        QVERIFY(!controller.dispatchKeyEvent(&ctrlPress));

        // The matching key press is consumed.
        QKeyEvent kPress(QEvent::KeyPress, Qt::Key_K, Qt::ControlModifier);
        QVERIFY(controller.dispatchKeyEvent(&kPress));
        QCOMPARE(triggered, 1);

        // Its release is consumed as well.
        QKeyEvent kRelease(QEvent::KeyRelease, Qt::Key_K, Qt::ControlModifier);
        QVERIFY(controller.dispatchKeyEvent(&kRelease));

        QKeyEvent ctrlRelease(QEvent::KeyRelease, Qt::Key_Control, Qt::NoModifier);
        QVERIFY(!controller.dispatchKeyEvent(&ctrlRelease));

        QCOMPARE(triggered, 1);
    }

    void testModifierOnlyCancelledByPointer()
    {
        ShortcutController controller;
        int triggered = 0;
        connect(&controller, &ShortcutController::actionTriggered, this,
                [&triggered](ShortcutAction, const QString &, bool, ShortcutController::KeyFlags) {
            ++triggered;
        });

        QCOMPARE(controller.registerKey(QStringLiteral("meta-action"), QStringLiteral("Meta"),
                                        ShortcutController::KeyRelease, ShortcutAction::Notify),
                 0u);

        // Pointer interaction cancels the pending modifier-only candidate, so
        // "tap the Win key while bumping the mouse" must not fire the shortcut.
        QKeyEvent press(QEvent::KeyPress, Qt::Key_Meta, Qt::MetaModifier);
        QVERIFY(!controller.dispatchKeyEvent(&press));

        controller.cancelModifierOnly();

        QKeyEvent release(QEvent::KeyRelease, Qt::Key_Meta, Qt::NoModifier);
        QVERIFY(!controller.dispatchKeyEvent(&release));
        QCOMPARE(triggered, 0);

        // A fresh tap after the cancel still fires the shortcut.
        QKeyEvent press2(QEvent::KeyPress, Qt::Key_Meta, Qt::MetaModifier);
        QVERIFY(!controller.dispatchKeyEvent(&press2));

        QKeyEvent release2(QEvent::KeyRelease, Qt::Key_Meta, Qt::NoModifier);
        QVERIFY(!controller.dispatchKeyEvent(&release2));
        QCOMPARE(triggered, 1);
    }

    void cleanupTestCase()
    {
        m_server->deleteLater();
        m_server = nullptr;
    }
};

QTEST_MAIN(ShortcutTest)
#include "main.moc"
