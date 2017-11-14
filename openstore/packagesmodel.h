#ifndef PACKAGESMODEL_H
#define PACKAGESMODEL_H

#include <QAbstractListModel>
#include <QJsonObject>

class PackageItem;

class PackagesModel : public QAbstractListModel
{
    Q_OBJECT
    Q_DISABLE_COPY(PackagesModel)
    Q_PROPERTY(int count READ rowCount NOTIFY updated)
    Q_PROPERTY(bool ready READ ready NOTIFY readyChanged)
    Q_PROPERTY(int updatesAvailableCount READ updatesAvailableCount NOTIFY updated)
    Q_PROPERTY(int notFromOpenStoreCount READ notFromOpenStoreCount NOTIFY updated)
    Q_PROPERTY(QString appStoreAppId MEMBER m_appStoreAppId NOTIFY appStoreAppIdChanged)
    Q_PROPERTY(bool appStoreUpdateAvailable READ appStoreUpdateAvailable NOTIFY appStoreUpdateAvailableChanged)
    Q_ENUMS(PackageStatus)

public:
    enum Roles {
        RoleName,
        RoleAppId,
        RoleIcon,
        RolePackageStatus,
    };

    enum PackageStatus {
        PackageUpdateAvailable,
        PackageIsNotFromOpenStore,
        PackageIsLatest
    };

    struct LocalPackageItem {
        QString name;
        QString appId;
        QString icon;
        PackageStatus status;
    };

    explicit PackagesModel(QAbstractListModel *parent = 0);

    QHash<int, QByteArray> roleNames() const Q_DECL_OVERRIDE;
    int rowCount(const QModelIndex & parent = QModelIndex()) const Q_DECL_OVERRIDE;
    QVariant data(const QModelIndex & index, int role = Qt::DisplayRole) const Q_DECL_OVERRIDE;

    bool ready() const { return m_ready; }
    int updatesAvailableCount() const;
    int notFromOpenStoreCount() const;

    bool appStoreUpdateAvailable() const { return m_appStoreUpdateAvailable; }

    Q_INVOKABLE void refresh();
    Q_INVOKABLE void showPackageDetails(const QString &appId);


Q_SIGNALS:
    void countChanged();
    void readyChanged();
    void updated();
    void packageDetailsReady(PackageItem* pkg);
    void appStoreAppIdChanged();
    void appStoreUpdateAvailableChanged();

private:
    QList<LocalPackageItem> m_list;
    QString m_appStoreAppId;
    bool m_appStoreUpdateAvailable;
    bool m_ready;
};

#endif // PACKAGESMODEL_H
