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

#include "binance.h"
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

    QVariantMap coinbaseBalances;
    QVariantMap coinbaseCurrencies;
    QVariantMap binanceBalances;
    QVariantMap binanceCurrencies;


    if (engine.rootObjects().isEmpty()){
        return -1;
    }
    QObject *topLevel = engine.rootObjects().value(0);
    QQuickWindow *window = qobject_cast<QQuickWindow*>(topLevel);
    

    auto updateBalance = [&](){
        double sum = 0.;
        if(!coinbaseBalances.isEmpty() && !coinbaseCurrencies.isEmpty()){
            
            double coinbaseSum = 0.;
            for(auto it = coinbaseBalances.begin(); it != coinbaseBalances.end(); ++it){
                auto value = it.value().toDouble() / coinbaseCurrencies[it.key()].toDouble();
                coinbaseSum += value;
                qDebug() << it.key() << " : " << it.value().toDouble() << "(" << value << "EUR) [1 " + it.key() + " @ " + QString::number((1./coinbaseCurrencies[it.key()].toDouble())) + "EUR]";
            }
            sum += coinbaseSum;
            qDebug() << "Coinbase portofolio : " << coinbaseSum;
        }
        if(!binanceBalances.isEmpty() && !binanceCurrencies.isEmpty()){
            double binanceSum = 0.;
            for(auto it = binanceBalances.begin(); it != binanceBalances.end(); ++it){
                if(binanceCurrencies.contains(it.key()+"EUR")){
                    auto value = it.value().toDouble() * binanceCurrencies[it.key()+"EUR"].toDouble();
                    binanceSum += value;
                    qDebug() << it.key() << " : " << it.value().toDouble() << "(" << value << "EUR) [1 " + it.key() + " @ " + QString::number((binanceCurrencies[it.key()+"EUR"].toDouble())) + "EUR]";
                }
                else{
                    for(auto it2 = binanceCurrencies.begin(); it2 != binanceCurrencies.end(); ++it2){
                        if(it2.key().startsWith(it.key())){
                            if(it2.key() == it.key()+"BTC"){
                                auto value = it.value().toDouble() * it2.value().toDouble() * binanceCurrencies["BTCEUR"].toDouble();
                                qDebug() << it.key() << " : " << it.value().toDouble() << "(" << value << "EUR) [1 " + it.key() + " @ " + QString::number(it2.value().toDouble()) + "BTC]";
                                binanceSum += value;
                                break;
                            }
                            else if(it2.key() == it.key()+"ETH"){
                                auto value = it.value().toDouble() * it2.value().toDouble() * binanceCurrencies["ETHEUR"].toDouble();
                                qDebug() << it.key() << " : " << it.value().toDouble() << "(" << value << "EUR) [1 " + it.key() + " @ " + QString::number(it2.value().toDouble()) + "ETH]";
                                binanceSum += value;
                                break;
                            }
                            else{
                                qDebug() << "Unkonwn pair " << it2.key();
                            }
                        }
                    }
                }
            }
            sum += binanceSum;
            qDebug() << "Binance portofolio : " << binanceSum;
        }
        window->setProperty("balance", sum);
        qDebug() << "total : " << sum;
    };

    QObject::connect(&coinbase, &Coinbase::onCurrencies, [&](QVariantMap currencyMap){
        coinbaseCurrencies = currencyMap;
        updateBalance();
    });

    QObject::connect(&coinbase, &Coinbase::onAccounts, [&](QVariantMap accountsMap){
        coinbaseBalances = accountsMap;
        updateBalance();
    });

    QTimer *currencyTimer = new QTimer(&app);
    QObject::connect(currencyTimer, &QTimer::timeout, &coinbase, &Coinbase::getCurrencies);

    QTimer *balanceTimer = new QTimer(&app);
    QObject::connect(balanceTimer, &QTimer::timeout, &coinbase, &Coinbase::getAccounts);

    QTimer *coinbaseGrantTimer = new QTimer(&app);
    coinbaseGrantTimer->setSingleShot(true);
    QObject::connect(coinbaseGrantTimer, &QTimer::timeout, [&](){
        coinbase.grant();
    });
    coinbaseGrantTimer->start(refreshToken.isEmpty() ? 0 : 3000);

    QObject::connect(&coinbase, &Coinbase::onAccessGranted, [&](){
        coinbase.getAccounts();
        coinbase.getCurrencies();
        balanceTimer->start(300000);
        currencyTimer->start(60000);
        auto refreshToken = coinbase.refreshToken();
        qDebug() << "Coinbase refresh token : " << refreshToken;
        settings.setValue(SETTINGS_COINBASE_REFRESH_TOKEN, refreshToken);
        coinbaseGrantTimer->stop();
    });

    Binance binance(apiKey, apiSecret);

    QObject::connect(&binance, &Binance::onAccounts, [&](QVariantMap accountsMap){
        binanceBalances = accountsMap;
        updateBalance();
    });

    QObject::connect(&binance, &Binance::onCurrencies, [&](QVariantMap currenciesMap){
        binanceCurrencies = currenciesMap;
        updateBalance();
    });

    QTimer *binanceCurrencyTimer = new QTimer(&app);
    QObject::connect(binanceCurrencyTimer, &QTimer::timeout, &binance, &Binance::getCurrencies);
    binanceCurrencyTimer->start(60000);
    
    

    QTimer *binanceBalanceTimer = new QTimer(&app);
    QObject::connect(binanceBalanceTimer, &QTimer::timeout, &binance, &Binance::getAccounts);
    binanceBalanceTimer->start(300000);

    binance.getAccounts();
    binance.getCurrencies();

    return app.exec();
}
