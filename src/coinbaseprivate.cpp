#include <QObject>

#include "coinbaseprivate.h"

#include <iostream>

#include <QtNetworkAuth/QOAuth2AuthorizationCodeFlow>
#include <QtNetworkAuth/QOAuthHttpServerReplyHandler>
#include <QNetworkReply>
#include <QDesktopServices>
#include <QNetworkProxy>
#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonObject>
#include <QFile>
#include <QNetworkRequest>

#define ENABLE_API_RESPONSE_DUMP 1
#define VERBOSE 1

constexpr auto clientId = COINBASE_CLIENT_ID;
constexpr auto clientSecret = COINBASE_CLIENT_SECRET;
constexpr auto redirectUri = "http://localhost:8080/auth";
constexpr auto authorizeUrl = "https://www.coinbase.com/oauth/authorize";
constexpr auto tokenUrl = "https://www.coinbase.com/oauth/token";
constexpr auto apiVersion = "2021-06-04";

class CoinbaseNetworkAccessManager : public QNetworkAccessManager{
public:
    CoinbaseNetworkAccessManager(QObject* parent):QNetworkAccessManager(parent){}
protected:
    virtual QNetworkReply* createRequest(QNetworkAccessManager::Operation op, const QNetworkRequest &originalReq, QIODevice *outgoingData = nullptr)override{
        if(originalReq.url().toString().startsWith("https://api.coinbase.com/v2/")){
            QNetworkRequest r(originalReq);
            r.setRawHeader("CB-VERSION", apiVersion);
            return QNetworkAccessManager::createRequest(op,r,outgoingData);
        }
        return QNetworkAccessManager::createRequest(op,originalReq,outgoingData);
    }
};


CoinbasePrivate::CoinbasePrivate(QString refreshToken, QObject *parent)
    : QObject(parent)
    , networkManager(new CoinbaseNetworkAccessManager(this))
    , coinbase(new QOAuth2AuthorizationCodeFlow(networkManager, this))
{

    coinbase->setScope("wallet:accounts:read");

    QObject::connect(coinbase, &QOAuth2AuthorizationCodeFlow::authorizeWithBrowser, &QDesktopServices::openUrl);

    coinbase->setAuthorizationUrl(QUrl(authorizeUrl));
    coinbase->setClientIdentifier(clientId);
    coinbase->setAccessTokenUrl(QUrl(tokenUrl));
    coinbase->setClientIdentifierSharedKey(clientSecret);
    coinbase->setRefreshToken(refreshToken);

    const auto port = static_cast<quint16>(QUrl(redirectUri).port());

    auto replyHandler = new QOAuthHttpServerReplyHandler(port, this);
    replyHandler->setCallbackPath("/auth/");
    coinbase->setReplyHandler(replyHandler);

    coinbase->setModifyParametersFunction([](QAbstractOAuth::Stage stage, QVariantMap* data){
        if(stage == QAbstractOAuth::Stage::RequestingAuthorization){
            data->insert("account", "all");
        }
    });

    QObject::connect(coinbase, &QOAuth2AuthorizationCodeFlow::granted, this, &CoinbasePrivate::onAccessGranted);

    coinbase->refreshAccessToken();
    
#if VERBOSE
    QObject::connect(coinbase, &QOAuth2AuthorizationCodeFlow::expirationAtChanged, [&](){
        qDebug() << QString("QOAuth2AuthorizationCodeFlow::expirationAtChanged -> ") + coinbase->expirationAt().toString();
    });
#endif
}

CoinbasePrivate::~CoinbasePrivate(){
}

QString CoinbasePrivate::refreshToken() const{
    return coinbase->refreshToken();
}

void CoinbasePrivate::grant(){
    coinbase->grant();
}

void CoinbasePrivate::getAccounts(){
    auto accounts = coinbase->get(QUrl("https://api.coinbase.com/v2/accounts?&limit=100"));

    QObject::connect(accounts, &QNetworkReply::finished, [=](){
        if( accounts->error() == QNetworkReply::NoError){
            auto response = accounts->readAll();
            auto jsonResponse = QJsonDocument::fromJson(response);

#if ENABLE_API_RESPONSE_DUMP
            QFile dump("accounts_coinbase.json");
            dump.open(QFile::WriteOnly);
            dump.write(jsonResponse.toJson(QJsonDocument::Indented));
            dump.close();
#endif

            QVariantMap accountsMap;

            for(auto object: jsonResponse["data"].toArray()){
                auto balance = object.toObject()["balance"].toObject();
                auto amount = balance["amount"].toString().toDouble();
                auto currency = balance["currency"].toString();
                if(amount > 0)
                {
                    accountsMap.insert(currency, amount);
                }
            }
            emit onAccounts(accountsMap);
        }
    });
}

void CoinbasePrivate::getCurrencies(){
        auto currencies = coinbase->get(QUrl("https://api.coinbase.com/v2/exchange-rates?currency=EUR"));

        QObject::connect(currencies, &QNetworkReply::finished, [=](){
            if( currencies->error() == QNetworkReply::NoError){
                auto response = currencies->readAll();
                auto jsonResponse = QJsonDocument::fromJson(response);

#if ENABLE_API_RESPONSE_DUMP
                QFile dump("currencies_coinbase.json");
                dump.open(QFile::WriteOnly);
                dump.write(jsonResponse.toJson(QJsonDocument::Indented));
                dump.close();
#endif
                QVariantMap currenciesMap;
                auto rates = jsonResponse["data"].toObject()["rates"].toObject();
                for(auto it = rates.begin(); it != rates.end(); ++it){
                    currenciesMap.insert(it.key(), it.value().toString().toDouble());
                }
                emit onCurrencies(currenciesMap);
            }
        });
}