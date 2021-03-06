/*
    Call Recorder for SailfishOS
    Copyright (C) 2014-2015 Dmitriy Purgin <dpurgin@gmail.com>

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "settings.h"

#include <QCoreApplication>
#include <QAudioDeviceInfo>
#include <QAudioFormat>
#include <QDebug>
#include <QDir>
#include <QSettings>
#include <QStandardPaths>
#include <QStringBuilder>

class Settings::SettingsPrivate
{
    friend class Settings;

    QString configPath()
    {
        return QString(QStandardPaths::writableLocation(QStandardPaths::ConfigLocation) %
                       QLatin1Char('/') % qApp->applicationName() %
                       QLatin1String("/callrecorder.ini"));
    }

    void readSettings()
    {
        QSettings settings(configPath(), QSettings::IniFormat);

        inputDeviceName = settings.value("deviceName", "source.primary").toString();
        outputLocation = settings.value("outputLocation",
                                        QString(QStandardPaths::writableLocation(QStandardPaths::DataLocation) %
                                                QLatin1String("/data"))).toString();                

        QString operationModeStr = settings.value("operationMode", "blacklist").toString();
        operationMode = (operationModeStr == "whitelist"? Settings::WhiteList: Settings::BlackList);

        settings.beginGroup("encoder");
            sampleRate = settings.value("sampleRate", 32000).toInt();
            sampleSize = settings.value("sampleSize", 16).toInt();
            compression = settings.value("compression", 8).toInt();
        settings.endGroup();

        settings.beginGroup("ui");
            locale = settings.value("locale", "system").toString();
        settings.endGroup();

        settings.beginGroup("storage");
            limitStorage = settings.value("limitStorage", false).toBool();
            maxStorageAge = settings.value("maxStorageAge", 365).toInt();
            maxStorageSize = settings.value("maxStorageSize", 1024).toLongLong(); // 1GB
            requireApproval = settings.value("requireApproval", false).toBool();
        settings.endGroup();
    }

    void saveSettings()
    {
        QSettings settings(configPath(), QSettings::IniFormat);

        settings.setValue("deviceName", inputDeviceName);
        settings.setValue("outputLocation", outputLocation);
        settings.setValue("operationMode",
                          operationMode == Settings::WhiteList?
                              "whitelist":
                              "blacklist");

        settings.beginGroup("encoder");
            settings.setValue("sampleRate", sampleRate);
            settings.setValue("sampleSize", sampleSize);
            settings.setValue("compression", compression);
        settings.endGroup();

        settings.beginGroup("ui");
            settings.setValue("locale", locale);
        settings.endGroup();

        settings.beginGroup("storage");
            settings.setValue("limitStorage", limitStorage);
            settings.setValue("maxStorageAge", maxStorageAge);
            settings.setValue("maxStorageSize", maxStorageSize);
            settings.setValue("requireApproval", requireApproval);
        settings.endGroup();
    }

    QAudioDeviceInfo inputDevice;

    QString inputDeviceName;

    QString locale;

    Settings::OperationMode operationMode;

    QString outputLocation;

    int compression;
    int sampleRate;
    int sampleSize;

    bool limitStorage;
    int maxStorageAge;
    int maxStorageSize;
    bool requireApproval;
};

Settings::Settings(QObject* parent)
    : QObject(parent),
      d(new SettingsPrivate)
{
    qDebug();

    d->readSettings();

    foreach (QAudioDeviceInfo deviceInfo, QAudioDeviceInfo::availableDevices(QAudio::AudioInput))
    {
        if (deviceInfo.deviceName() == d->inputDeviceName)
        {
            d->inputDevice = deviceInfo;
            break;
        }
    }

    if (d->inputDevice.isNull())
    {
        qDebug() << ": unable to find " << d->inputDeviceName;

        d->inputDevice = QAudioDeviceInfo::defaultInputDevice();

        qDebug() << ": fallen back to " << d->inputDevice.deviceName();
    }

    QDir outputLocationDir(outputLocation());

    if (!outputLocationDir.exists())
        QDir().mkpath(outputLocation());

    createNoMediaFile();
}

Settings::~Settings()
{
    qDebug();
}

QAudioFormat Settings::audioFormat() const
{
    QAudioFormat format;
    format.setByteOrder(QAudioFormat::LittleEndian);
    format.setChannelCount(1);
    format.setCodec("audio/pcm");
    format.setSampleRate(d->sampleRate);
    format.setSampleSize(d->sampleSize);
    format.setSampleType(QAudioFormat::SignedInt);

    return d->inputDevice.nearestFormat(format);
}

void Settings::createNoMediaFile()
{
    // create a .nomedia file in output location
    // this will tell tracker-miner that the output location
    // doesn't contain media files

    QFile nomediaFile(QDir(outputLocation()).filePath(".nomedia"));

    if (!nomediaFile.exists())
    {
        if (!nomediaFile.open(QFile::WriteOnly | QFile::Truncate))
        {
            qWarning() << "unable to open " <<
                          nomediaFile.fileName() <<
                          ": " <<
                          nomediaFile.errorString();
        }

        nomediaFile.close();
    }
}

int Settings::compression() const
{
    return d->compression;
}

QAudioDeviceInfo Settings::inputDevice() const
{
    return d->inputDevice;
}

bool Settings::limitStorage() const
{
    return d->limitStorage;
}

QString Settings::locale() const
{
    return d->locale;
}

int Settings::maxStorageAge() const
{
    return d->maxStorageAge;
}

int Settings::maxStorageSize() const
{
    return d->maxStorageSize;
}

Settings::OperationMode Settings::operationMode() const
{
    return d->operationMode;
}

QString Settings::outputLocation() const
{
    return d->outputLocation;
}

void Settings::reload()
{
    qDebug();

    d->readSettings();
}

bool Settings::requireApproval() const
{
    return d->requireApproval;
}

int Settings::sampleRate() const
{
    return d->sampleRate;
}

void Settings::save()
{
    qDebug();

    d->saveSettings();
}

void Settings::setCompression(int compression)
{
    if (d->compression != compression)
    {
        d->compression = compression;

        emit compressionChanged(compression);
        emit settingsChanged();
    }
}

void Settings::setLimitStorage(bool limitStorage)
{
    if (d->limitStorage != limitStorage)
    {
        d->limitStorage = limitStorage;

        emit limitStorageChanged(limitStorage);
        emit settingsChanged();
    }
}

void Settings::setLocale(const QString& locale)
{
    if (d->locale != locale)
    {
        d->locale = locale;

        emit localeChanged(locale);
        emit settingsChanged();
    }
}

void Settings::setMaxStorageAge(int age)
{
    if (d->maxStorageAge != age)
    {
        d->maxStorageAge = age;

        emit maxStorageAgeChanged(age);
        emit settingsChanged();
    }
}

void Settings::setMaxStorageSize(int size)
{
    if (d->maxStorageSize != size)
    {
        d->maxStorageSize = size;

        emit maxStorageSizeChanged(size);
        emit settingsChanged();
    }
}

void Settings::setOperationMode(OperationMode operationMode)
{
    if (d->operationMode != operationMode)
    {
        d->operationMode = operationMode;

        emit operationModeChanged(operationMode);
        emit settingsChanged();
    }
}

void Settings::setOutputLocation(const QString& outputLocation)
{
    if (d->outputLocation != outputLocation)
    {
        d->outputLocation = outputLocation;

        createNoMediaFile();

        emit outputLocationChanged(outputLocation);
        emit settingsChanged();
    }
}

void Settings::setRequireApproval(bool requireApproval)
{
    if (d->requireApproval != requireApproval)
    {
        d->requireApproval = requireApproval;

        emit requireApprovalChanged(requireApproval);
        emit settingsChanged();
    }
}

void Settings::setSampleRate(int sampleRate)
{
    if (d->sampleRate != sampleRate)
    {
        d->sampleRate = sampleRate;

        emit sampleRateChanged(sampleRate);
        emit settingsChanged();
    }
}

QDebug operator<<(QDebug dbg, Settings::OperationMode state)
{
    dbg << (state == Settings::BlackList? "Black List": "White List");

    return dbg;
}
