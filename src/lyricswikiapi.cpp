/*
  The MIT License (MIT)

  Copyright (c) 2015 Andrea Scarpino <me@andreascarpino.it>

  Permission is hereby granted, free of charge, to any person obtaining a copy
  of this software and associated documentation files (the "Software"), to deal
  in the Software without restriction, including without limitation the rights
  to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
  copies of the Software, and to permit persons to whom the Software is
  furnished to do so, subject to the following conditions:

  The above copyright notice and this permission notice shall be included in all
  copies or substantial portions of the Software.

  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
  AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
  LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
  OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
  SOFTWARE.
*/

#include "lyricswikiapi.h"

#include <QDebug>
#include <QJsonDocument>
#include <QJsonObject>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QWebElement>
#include <QWebFrame>
#include <QWebPage>
#include <QUrl>
#include <QUrlQuery>

#include "lyric.h"

const static QString API_URL = QStringLiteral("http://lyrics.wikia.com/api.php");

LyricsWikiAPI::LyricsWikiAPI(QObject *parent) :
    QObject(parent)
  , network(new QNetworkAccessManager(this))
{
}

LyricsWikiAPI::~LyricsWikiAPI()
{
    qDeleteAll(lyrics.keys());
    qDeleteAll(lyrics.values());
    delete network;
}

void LyricsWikiAPI::getLyricBypassApi(const QString &artist, const QString &song)
{
    qDebug() << "Requesting lyric bypassing APIs for artist" << artist << ", song" << song;

    QUrl url(QStringLiteral("http://lyrics.wikia.com"));

    QString artNoSpaces(artist);
    artNoSpaces.replace(QChar::Space, "_");
    QString songNoSpaces(song);
    songNoSpaces.replace(QChar::Space, "_");

    url.setPath(artNoSpaces + QChar::fromLatin1(':') + songNoSpaces);

    QNetworkRequest req(url);
    QNetworkReply* reply = network->get(req);

    Lyric* lyric = new Lyric();
    lyric->setArtist(artist);
    lyric->setTitle(song);
    lyrics.insert(reply, lyric);

    connect(reply, &QNetworkReply::finished, this, &LyricsWikiAPI::onGetLyricPageResult);
}

void LyricsWikiAPI::getLyric(const QString &artist, const QString &song)
{
    qDebug() << "Requesting lyric for artist" << artist << ", song" << song;
    QUrl url(API_URL);

    QString artNoSpaces(artist);
    artNoSpaces.replace(QChar::Space, "_");
    QString songNoSpaces(song);
    songNoSpaces.replace(QChar::Space, "_");

    QUrlQuery query;
    query.addQueryItem(QStringLiteral("func"), QStringLiteral("getSong"));
    query.addQueryItem(QStringLiteral("artist"), artNoSpaces);
    query.addQueryItem(QStringLiteral("song"), songNoSpaces);
    query.addQueryItem(QStringLiteral("fmt"), QStringLiteral("realjson"));
    url.setQuery(query);

    QNetworkRequest req(url);
    QNetworkReply* reply = network->get(req);

    connect(reply, &QNetworkReply::finished, this, &LyricsWikiAPI::onGetLyricResult);
}

void LyricsWikiAPI::onGetLyricResult()
{
    QNetworkReply* reply = qobject_cast<QNetworkReply*>(QObject::sender());

    bool found = false;
    Lyric* lyric;

    if (reply->error() != QNetworkReply::NoError) {
        qCritical() << "Cannot fetch lyric";
    } else {
        QJsonDocument json = QJsonDocument::fromJson(reply->readAll());
        if (!json.isNull()) {
            qDebug() << "Got lyric:\n" << json;

            QJsonObject jsonObj = json.object();

            if (jsonObj.value("lyrics").toString() != QStringLiteral("Not found")) {
                lyric = new Lyric();
                lyric->setArtist(jsonObj.value("artist").toString());
                lyric->setTitle(jsonObj.value("song").toString());

                const QUrl url(jsonObj.value("url").toString());
                getLyricText(url, lyric);
            } else {
                qDebug() << "No lyric found";
            }
        } else {
            qCritical() << "Got an invalid JSON!";
        }
    }

    if (!found) {
        emit lyricFetched(lyric, found);
    }

    reply->deleteLater();
}

void LyricsWikiAPI::onGetLyricPageResult()
{
    QNetworkReply* reply = qobject_cast<QNetworkReply*>(QObject::sender());

    bool found = false;
    Lyric* lyric;

    if (reply->error() != QNetworkReply::NoError) {
        qCritical() << "Cannot fetch lyric";
    } else {
        QWebPage page;
        page.mainFrame()->setHtml(reply->readAll());
        QWebElement parse = page.mainFrame()->documentElement();
        QWebElement result = parse.findFirst("div[class=lyricbox]");

        lyric = lyrics.value(reply);
        if (!lyric) {
            qCritical() << "Got an invalid lyric object!";
        } else {
            qDebug() << result.toPlainText();

            lyric->setText(result.toPlainText());
            lyrics.remove(reply);

            found = true;
        }
    }

    emit lyricFetched(lyric, found);

    reply->deleteLater();
}

void LyricsWikiAPI::getLyricText(const QUrl &url, Lyric *lyric)
{
    qDebug() << "Requesting lyric page" << url;
    QNetworkRequest req(url);
    QNetworkReply* reply = network->get(req);
    lyrics.insert(reply, lyric);

    connect(reply, &QNetworkReply::finished, this, &LyricsWikiAPI::onGetLyricPageResult);
}
