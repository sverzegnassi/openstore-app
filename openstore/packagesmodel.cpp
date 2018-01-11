#include "packagesmodel.h"
#include "package.h"
#include "platformintegration.h"
#include "openstorenetworkmanager.h"
#include "packagescache.h"

#include <QJsonDocument>
#include <QJsonArray>

// For desktop file / scope settings parsing
#include <QSettings>
#include <QFileInfo>
#include <QDir>

#define MODEL_START_REFRESH() m_ready = false; Q_EMIT readyChanged();
#define MODEL_END_REFRESH() m_ready = true; Q_EMIT readyChanged();

PackagesModel::PackagesModel(QAbstractListModel * parent)
    : QAbstractListModel(parent)
    , m_ready(false)
    , m_appStoreUpdateAvailable(false)
{
    connect(PlatformIntegration::instance(), &PlatformIntegration::updated, this, &PackagesModel::refresh);
    connect(PackagesCache::instance(), &PackagesCache::updatingCacheChanged, this, &PackagesModel::refresh);

    refresh();
}

QHash<int, QByteArray> PackagesModel::roleNames() const
{
    QHash<int, QByteArray> roles;

    roles.insert(RoleName, "name");
    roles.insert(RoleAppId, "appId");
    roles.insert(RoleIcon, "icon");
    roles.insert(RolePackageStatus, "packageStatus");

    return roles;
}

int PackagesModel::rowCount(const QModelIndex & parent) const
{
    Q_UNUSED(parent)
    return m_list.count();
}

QVariant PackagesModel::data(const QModelIndex & index, int role) const
{
    if (index.row() < 0 || index.row() > rowCount())
        return QVariant();

    auto pkg = m_list.at(index.row());

    switch (role) {
    case RoleName:
        return pkg.name;
    case RoleAppId:
        return pkg.appId;
    case RoleIcon:
        return pkg.icon;
    case RolePackageStatus:
        return pkg.status;

    default:
        return QVariant();
    }
}

int PackagesModel::updatesAvailableCount() const
{
    int result = 0;

    Q_FOREACH (const LocalPackageItem &pkg, m_list) {
        if (pkg.status == PackageStatus::PackageUpdateAvailable) {
            ++result;
        }
    }

    return result;
}

int PackagesModel::notFromOpenStoreCount() const
{
    int result = 0;

    Q_FOREACH (const LocalPackageItem &pkg, m_list) {
        if (pkg.status == PackageStatus::PackageIsNotFromOpenStore) {
            ++result;
        }
    }

    return result;
}

void PackagesModel::refresh()
{
    qDebug() << Q_FUNC_INFO << "called";

    MODEL_START_REFRESH();

    beginResetModel();
    m_list.clear();
    endResetModel();

    const QVariantList &clickDb = PlatformIntegration::instance()->clickDb();

    beginInsertRows(QModelIndex(), m_list.count(), m_list.count() + PackagesCache::instance()->numberOfInstalledAppsInStore() - 1);
    Q_FOREACH(const QVariant &pkg, clickDb) {
        QVariantMap map = pkg.toMap();

        LocalPackageItem pkgItem;
        pkgItem.appId = map.value("name").toString();

        if (PackagesCache::instance()->getRemoteAppRevision(pkgItem.appId) != -1) {
            pkgItem.name = map.value("title").toString();

            const int localRevision = PackagesCache::instance()->getLocalAppRevision(pkgItem.appId);
            const int remoteRevision = PackagesCache::instance()->getRemoteAppRevision(pkgItem.appId);

            if (pkgItem.appId.contains("com.ubuntu"))
                qDebug() << Q_FUNC_INFO << pkgItem.appId << localRevision << remoteRevision;

            pkgItem.status = (localRevision < 1) ? PackageStatus::PackageIsNotFromOpenStore
                                                 : remoteRevision > localRevision ? PackageStatus::PackageUpdateAvailable
                                                                                  : PackageStatus::PackageIsLatest;

            //pkgItem.icon = map.value("icon").toString();
            if (pkgItem.icon.isEmpty()) {
                const QString &directory = map.value("_directory").toString();

                const QVariantMap &hooks = map.value("hooks").toMap();
                Q_FOREACH(const QString &hook, hooks.keys()) {
                    const QVariantMap &h = hooks.value(hook).toMap();

                    const QString &desktop = h.value("desktop").toString();
                    //const QString &scope = h.value("scope").toString();
                    if (!desktop.isEmpty()) {
                        const QString &desktopFile = directory + QDir::separator() + desktop;
                        QSettings appInfo(desktopFile, QSettings::IniFormat);
                        pkgItem.icon = directory + QDir::separator() + appInfo.value("Desktop Entry/Icon").toString();
                        break;
                    } /*else if (!scope.isEmpty()) {
                            const QString &scopeFile = directory + QDir::separator() + scope + QDir::separator() + pkgItem.appId + "_" + scope + ".ini";
                            QSettings appInfo(scopeFile, QSettings::IniFormat);
                            QFileInfo fileInfo(scopeFile);

                            pkgItem.icon = fileInfo.absolutePath() + QDir::separator() + appInfo.value("ScopeConfig/Icon").toString();
                            break;
                        }*/
                }
            }

            if (!pkgItem.icon.isEmpty()) {
                pkgItem.icon = pkgItem.icon.prepend("file://");
            }

            m_list.append(pkgItem);

            // Check if there's an update for OpenStore
            if (pkgItem.appId == m_appStoreAppId) {
                m_appStoreUpdateAvailable = bool(pkgItem.status == PackageStatus::PackageUpdateAvailable);
                Q_EMIT appStoreUpdateAvailableChanged();
            }
        }
    }

    endInsertRows();

    Q_EMIT updated();
    MODEL_END_REFRESH();
}
