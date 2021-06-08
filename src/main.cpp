#include <sqlite3.h>

#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQuickItem>
#include <QQuickWindow>
#include <QSslSocket>
#include <QVariantMap>
#include <QQmlContext>
#include <QTimer>
#include <QSettings>

#include "coinbase.h"

constexpr auto SETTINGS_COINBASE_REFRESH_TOKEN = "coinbase/refreshToken";

int main(int argc, char *argv[]) {
    QGuiApplication app(argc, argv);

    app.setOrganizationName("Tib's Lab");
    app.setOrganizationDomain("tibslab.com");
    app.setApplicationName("Crypto Watch");

    QSettings settings;
    
    QQmlApplicationEngine engine;
    const QUrl url(QStringLiteral("qrc:/main.qml"));
    QObject::connect(
        &engine, &QQmlApplicationEngine::objectCreated,
        &app, [url](QObject *obj, const QUrl &objUrl) {
            if (!obj && url == objUrl)
                QCoreApplication::exit(-1);
        },
        Qt::QueuedConnection);
    engine.load(url);

    // --- Test SQLite3 (Copy DLLs)
    sqlite3 *db;
    char *zErrMsg = 0;
    int rc;

    rc = sqlite3_open("test.db", &db);

    if (rc) {
        fprintf(stderr, "Can't open database: %s\n", sqlite3_errmsg(db));
        return (0);
    } else {
        fprintf(stderr, "Opened database successfully\n");
    }
    sqlite3_close(db);
    // system("pause");

    qDebug() << QSslSocket::sslLibraryBuildVersionString();
    qDebug() << QSslSocket::supportsSsl();
    qDebug() << QSslSocket::sslLibraryVersionString();

    QString refreshToken = settings.value(SETTINGS_COINBASE_REFRESH_TOKEN).toString();
    qDebug() << "using coinbase refresh token " << refreshToken;
    Coinbase coinbase(refreshToken);

    QVariantMap balances;
    QVariantMap currencies;

    if (engine.rootObjects().isEmpty()){
        return -1;
    }
    QObject *topLevel = engine.rootObjects().value(0);
    QQuickWindow *window = qobject_cast<QQuickWindow*>(topLevel);
    

    auto updateBalance = [&](){
        if(!balances.isEmpty() && !currencies.isEmpty()){
            double sum = 0.;
            for(auto it = balances.begin(); it != balances.end(); ++it){
                auto value = it.value().toDouble() / currencies[it.key()].toDouble();
                sum += value;
                qDebug() << it.key() << " : " << it.value().toDouble() << "(" << value << "EUR)";
            }
            window->setProperty("balance", sum);
            qDebug() << "total : " << sum;
        }
    };

    QObject::connect(&coinbase, &Coinbase::onCurrencies, [&](QVariantMap currencyMap){
        currencies = currencyMap;
        updateBalance();
    });

    QObject::connect(&coinbase, &Coinbase::onAccounts, [&](QVariantMap accountsMap){
        balances = accountsMap;
        updateBalance();
    });

    QTimer *currencyTimer = new QTimer(&app);
    QObject::connect(currencyTimer, &QTimer::timeout, &coinbase, &Coinbase::getCurrencies);

    QTimer *balanceTimer = new QTimer(&app);
    QObject::connect(balanceTimer, &QTimer::timeout, &coinbase, &Coinbase::getAccounts);

    QTimer *coinbaseGrant = new QTimer(&app);
    QObject::connect(coinbaseGrant, &QTimer::timeout, [&](){
        coinbase.grant();
    });
    coinbaseGrant->start(refreshToken.isEmpty() ? 0 : 3000);

    QObject::connect(&coinbase, &Coinbase::onAccessGranted, [&](){
        coinbase.getAccounts();
        coinbase.getCurrencies();
        balanceTimer->start(300000);
        currencyTimer->start(60000);
        auto refreshToken = coinbase.refreshToken();
        qDebug() << "Coinbase refresh token : " << refreshToken;
        settings.setValue(SETTINGS_COINBASE_REFRESH_TOKEN, refreshToken);
        coinbaseGrant->stop();
    });

    

    return app.exec();
}
