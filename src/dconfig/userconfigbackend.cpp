// Copyright (C) 2026 UnionTech Software Technology Co., Ltd.
// SPDX-License-Identifier: Apache-2.0 OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "userconfigbackend.h"

#include "common/treelandlogging.h"

#include <QDBusArgument>
#include <QDBusObjectPath>
#include <QDBusReply>
#include <QDBusVariant>

namespace {
const QString service = QStringLiteral("org.desktopspec.ConfigManager");
const QString rootInterface = QStringLiteral("org.desktopspec.ConfigManager");
const QString managerInterface = QStringLiteral("org.desktopspec.ConfigManager.Manager");

QVariant decodeValue(const QVariant &value)
{
    if (value.metaType() == QMetaType::fromType<QDBusVariant>())
        return decodeValue(value.value<QDBusVariant>().variant());
    if (value.metaType() != QMetaType::fromType<QDBusArgument>())
        return value;
    const auto argument = value.value<QDBusArgument>();
    if (argument.currentType() == QDBusArgument::MapType) {
        QVariantMap map;
        argument >> map;
        for (auto it = map.begin(); it != map.end(); ++it)
            it.value() = decodeValue(it.value());
        return map;
    }
    if (argument.currentType() == QDBusArgument::ArrayType) {
        QVariantList list;
        argument >> list;
        for (auto &item : list)
            item = decodeValue(item);
        return list;
    }
    return value;
}
} // namespace

UserConfigBackend::UserConfigBackend(uint uid, const QString &name, const QString &subpath)
    : m_uid(uid)
    , m_name(name)
    , m_subpath(subpath)
{
    moveToThread(DTK_CORE_NAMESPACE::DConfig::globalThread());
}

UserConfigBackend::~UserConfigBackend()
{
    if (isValid())
        call(QStringLiteral("release"), {});
}

QDBusPendingReply<QDBusObjectPath>
UserConfigBackend::acquireManagerV2(const QString &appId) const
{
    auto request = QDBusMessage::createMethodCall(service,
                                                  QStringLiteral("/"),
                                                  rootInterface,
                                                  QStringLiteral("acquireManagerV2"));
    request.setArguments({ m_uid, appId, m_name, m_subpath });
    return QDBusConnection::systemBus().asyncCall(request);
}

bool UserConfigBackend::load(const QString &appId)
{
    if (m_manager)
        return isValid();
    auto reply = acquireManagerV2(appId);
    reply.waitForFinished();
    if (reply.isError() || reply.value().path().isEmpty()) {
        qCWarning(lcTlConfig) << "Cannot acquire user DConfig" << m_uid << m_name << m_subpath
                              << reply.error();
        return false;
    }
    m_manager = std::make_unique<QDBusInterface>(service,
                                                 reply.value().path(),
                                                 managerInterface,
                                                 QDBusConnection::systemBus());
    return isValid();
}

bool UserConfigBackend::isValid() const
{
    return m_manager && m_manager->isValid();
}

QStringList UserConfigBackend::keyList() const
{
    return m_manager->property("keyList").toStringList();
}

QDBusMessage UserConfigBackend::call(const QString &method, const QVariantList &arguments) const
{
    const auto reply = m_manager->callWithArgumentList(QDBus::Block, method, arguments);
    if (reply.type() == QDBusMessage::ErrorMessage)
        qCWarning(lcTlConfig) << "User DConfig request failed" << m_uid << m_name << m_subpath
                              << method << reply.errorName() << reply.errorMessage();
    return reply;
}

QVariant UserConfigBackend::value(const QString &key, const QVariant &fallback) const
{
    const QDBusReply<QDBusVariant> reply = call(QStringLiteral("value"), { key });
    return reply.isValid() ? decodeValue(reply.value().variant()) : fallback;
}

void UserConfigBackend::setValue(const QString &key, const QVariant &value)
{
    call(QStringLiteral("setValue"), { key, QVariant::fromValue(QDBusVariant(value)) });
}

void UserConfigBackend::reset(const QString &key)
{
    call(QStringLiteral("reset"), { key });
}

bool UserConfigBackend::isDefaultValue(const QString &key) const
{
    const QDBusReply<bool> reply = call(QStringLiteral("isDefaultValue"), { key });
    return reply.isValid() && reply.value();
}

bool UserConfigBackend::isReadOnly(const QString &key) const
{
    const QDBusReply<QString> reply = call(QStringLiteral("permissions"), { key });
    return !reply.isValid() || reply.value() == QLatin1String("readonly");
}

QString UserConfigBackend::name() const
{
    return QStringLiteral("UserDBusBackend");
}

void UserConfigBackend::subscribe(DTK_CORE_NAMESPACE::DConfig *config)
{
    if (!QDBusConnection::systemBus().connect(service,
                                              m_manager->path(),
                                              managerInterface,
                                              QStringLiteral("valueChanged"),
                                              config,
                                              SIGNAL(valueChanged(QString)))) {
        qCWarning(lcTlConfig) << "Cannot subscribe to user DConfig changes" << m_uid << m_name;
    }
    // The generated wrapper has now installed its DConfig listener. Refresh
    // after subscribing to cover changes during its asynchronous initialization.
    for (const auto &key : keyList())
        Q_EMIT config->valueChanged(key);
}
