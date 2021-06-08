#include "coinbase.h"

#include "coinbaseprivate.h"

Coinbase::Coinbase(QString refreshToken, QObject* parent)
    : QObject(parent)
    , coinbasePrivate(new CoinbasePrivate(refreshToken, this))
{
    QObject::connect(coinbasePrivate, &CoinbasePrivate::onAccessGranted, this, &Coinbase::onAccessGranted);
    QObject::connect(coinbasePrivate, &CoinbasePrivate::onAccounts, this, &Coinbase::onAccounts);
    QObject::connect(coinbasePrivate, &CoinbasePrivate::onCurrencies, this, &Coinbase::onCurrencies);
}

Coinbase::~Coinbase(){
}

void Coinbase::grant() const{
    coinbasePrivate->grant();
}

void Coinbase::getAccounts() const{
    coinbasePrivate->getAccounts();
}

void Coinbase::getCurrencies() const{
    coinbasePrivate->getCurrencies();
}

QString Coinbase::refreshToken() const{
    return coinbasePrivate->refreshToken();
}

