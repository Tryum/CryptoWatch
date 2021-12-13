#include "binanceprivate.h"

#include <QDateTime>
#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QMessageAuthenticationCode>
#include <QNetworkAccessManager>
#include <QNetworkProxy>
#include <QNetworkReply>
#include <QString>
#include <QUrl>
#include <QUrlQuery>

#define ENABLE_API_RESPONSE_DUMP 1

const QString baseUrl = "https://api.binance.com";
/*export api key and secret key to environment*/




BinancePrivate::BinancePrivate(QString key, QString secret, QObject* parent)
    : QObject(parent)
    , key(key)
    , secret(secret)
    , manager(new QNetworkAccessManager(this)) {
    
    
}

BinancePrivate::~BinancePrivate() {

}

QByteArray BinancePrivate::sign(const QUrlQuery& query){
    auto queryString = query.toString().toLocal8Bit();
    auto signature = QMessageAuthenticationCode::hash(queryString, secret.toLocal8Bit(), QCryptographicHash::Sha256).toHex();
    return signature;
}

void BinancePrivate::getAccounts(){
    QUrl url(baseUrl+"/api/v3/account");
    QUrlQuery query;
    query.addQueryItem("timestamp", QString::number(QDateTime::currentMSecsSinceEpoch()));
    

    query.addQueryItem("signature", sign(query));

    url.setQuery(query);

    qDebug() << "Url : " << url;

    QNetworkRequest request;
    request.setRawHeader("X-MBX-APIKEY", key.toLocal8Bit());
    request.setUrl(url);

    auto reply = manager->get(request);

    QObject::connect(reply, &QNetworkReply::finished, [=](){
        if( reply->error() == QNetworkReply::NoError){
            auto response = reply->readAll();
            auto jsonResponse = QJsonDocument::fromJson(response);

#if ENABLE_API_RESPONSE_DUMP
            QFile dump("accounts_binance.json");
            dump.open(QFile::WriteOnly);
            dump.write(jsonResponse.toJson(QJsonDocument::Indented));
            dump.close();
#endif

            QVariantMap accountsMap;

            for(auto object: jsonResponse.object()["balances"].toArray()){
                auto balance = object.toObject()["free"].toString().toDouble();
                auto currency = object.toObject()["asset"].toString();
                if(balance > 0)
                {
                    accountsMap.insert(currency, balance);
                }
            }
            emit onAccounts(accountsMap);
        }
        else{
            qDebug() << "reply error " << reply->errorString();
        }
    });
}

void BinancePrivate::getCurrencies(){
    QUrl url(baseUrl+"/api/v3/ticker/price");
    
    QNetworkRequest request;
    request.setUrl(url);

    auto reply = manager->get(request);

    QObject::connect(reply, &QNetworkReply::finished, [=](){
        if( reply->error() == QNetworkReply::NoError){
            auto response = reply->readAll();
            auto jsonResponse = QJsonDocument::fromJson(response);

#if ENABLE_API_RESPONSE_DUMP
            QFile dump("currencies_binance.json");
            dump.open(QFile::WriteOnly);
            dump.write(jsonResponse.toJson(QJsonDocument::Indented));
            dump.close();
#endif
                QVariantMap currenciesMap;
                auto rates = jsonResponse.array();
                for(auto object: jsonResponse.array()){
                    currenciesMap.insert(object.toObject()["symbol"].toString(), object.toObject()["price"].toString().toDouble());
                }
                emit onCurrencies(currenciesMap);
        }
        else{
            qDebug() << "reply error " << reply->errorString();
        }
    });
}