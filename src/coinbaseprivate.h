#pragma once

#include <QObject>
#include <QVariantMap>

class QOAuth2AuthorizationCodeFlow;
class CoinbaseNetworkAccessManager;

class CoinbasePrivate : public QObject {
    Q_OBJECT
public:
    CoinbasePrivate(QString refreshToken, QObject* parent);
    ~CoinbasePrivate();

    QString refreshToken() const;

public slots:
    void grant();
    void getAccounts();
    void getCurrencies();

signals:
    void onAccessGranted();
    void onAccounts(QVariantMap accounts);
    void onCurrencies(QVariantMap currencies);

private:
    CoinbaseNetworkAccessManager* networkManager;
    QOAuth2AuthorizationCodeFlow* coinbase;

};