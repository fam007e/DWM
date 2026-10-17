import QtQuick
import QtQuick.Layouts
import qs.core

ClickAwayPopup {
    id: root

    required property var weatherModel
    required property var panelWindow

    readonly property int cardWidth: 320
    readonly property int edgeMargin: Theme.rowSpacing

    visible: panelWindow !== null && panelWindow.screen !== null && weatherModel.visible
    targetWindow: panelWindow
    popupWidth: cardWidth
    popupHeight: weatherCard.implicitHeight
    popupX: panelWindow ? Math.max(edgeMargin, (panelWindow.width / 2) - (cardWidth / 2) + 60) : edgeMargin
    popupY: Theme.panelHeight
    onDismissed: weatherModel.close()
    onVisibleChanged: {
        if (visible) {
            Qt.callLater(function() {
                weatherCard.forceActiveFocus();
            });
        }
    }

    ShellSurface {
        id: weatherCard

        anchors.fill: parent
        implicitHeight: contentColumn.implicitHeight + Theme.cardPadding * 2
        margin: 6
        focus: true

        Keys.onPressed: function(event) {
            if (event.key === Qt.Key_Escape) {
                root.weatherModel.close();
                event.accepted = true;
            } else if (event.key === Qt.Key_R) {
                root.weatherModel.refresh();
                event.accepted = true;
            }
        }

        ColumnLayout {
            id: contentColumn

            anchors.fill: parent
            anchors.margins: Theme.cardPadding
            spacing: Theme.spacingMd

            // Header: City, Country and Refresh
            RowLayout {
                Layout.fillWidth: true
                spacing: Theme.spacingSm

                IconText {
                    text: ""
                    color: Theme.accent
                    font.pixelSize: Theme.scaledFontSize(14, 8)
                }

                UiText {
                    Layout.fillWidth: true
                    text: root.weatherModel.city + (root.weatherModel.country ? ", " + root.weatherModel.country : "")
                    color: Theme.textStrong
                    font.bold: true
                    font.pixelSize: Theme.panelFontSize
                    elide: Text.ElideRight
                }

                ShellButton {
                    label: ""
                    leadingLabel: "󰑓"
                    accessibleDescription: "Refresh weather"
                    onActivated: root.weatherModel.refresh()
                }
            }

            Rectangle {
                Layout.fillWidth: true
                height: 1
                color: Theme.border
            }

            // Big Temp & Icon (Centrally Aligned)
            ColumnLayout {
                Layout.fillWidth: true
                Layout.alignment: Qt.AlignHCenter
                spacing: 4

                RowLayout {
                    Layout.alignment: Qt.AlignHCenter
                    spacing: Theme.spacingMd

                    IconText {
                        text: root.weatherModel.icon
                        color: Theme.accent
                        font.pixelSize: Theme.scaledFontSize(40, 14)
                        Layout.alignment: Qt.AlignVCenter
                    }

                    RowLayout {
                        spacing: 2
                        Layout.alignment: Qt.AlignVCenter

                        UiText {
                            text: root.weatherModel.temp
                            color: Theme.textStrong
                            font.bold: true
                            font.pixelSize: Theme.scaledFontSize(36, 14)
                        }

                        UiText {
                            text: "°C"
                            color: Theme.textMuted
                            font.pixelSize: Theme.panelFontSize
                            Layout.alignment: Qt.AlignTop
                        }
                    }
                }

                UiText {
                    Layout.alignment: Qt.AlignHCenter
                    text: root.weatherModel.detailedDesc || root.weatherModel.desc
                    color: Theme.text
                    font.pixelSize: Theme.fontCaptionSize
                    horizontalAlignment: Text.AlignHCenter
                }

                // Min / Max row
                RowLayout {
                    Layout.alignment: Qt.AlignHCenter
                    spacing: Theme.spacingSm
                    visible: root.weatherModel.tempMin !== "--" && root.weatherModel.tempMax !== "--"

                    UiText {
                        text: "↓ " + root.weatherModel.tempMin + "°"
                        color: Theme.accent
                        font.pixelSize: Theme.tinyFontSize
                    }

                    UiText {
                        text: "•"
                        color: Theme.textMuted
                        font.pixelSize: Theme.tinyFontSize
                    }

                    UiText {
                        text: "↑ " + root.weatherModel.tempMax + "°"
                        color: Theme.warning
                        font.pixelSize: Theme.tinyFontSize
                    }
                }
            }

            Rectangle {
                Layout.fillWidth: true
                height: 1
                color: Theme.border
            }

            // Details Grid Row 1: Feels like, Humidity, Wind
            RowLayout {
                Layout.fillWidth: true
                spacing: Theme.spacingSm

                // Feels Like
                Rectangle {
                    Layout.fillWidth: true
                    Layout.preferredHeight: 52
                    color: Theme.surface
                    radius: Theme.controlRadius

                    ColumnLayout {
                        anchors.centerIn: parent
                        spacing: 2

                        UiText {
                            text: "Feels Like"
                            color: Theme.textMuted
                            font.pixelSize: Theme.tinyFontSize
                            Layout.alignment: Qt.AlignHCenter
                        }

                        UiText {
                            text: root.weatherModel.feels + "°C"
                            color: Theme.textStrong
                            font.bold: true
                            font.pixelSize: Theme.fontCaptionSize
                            Layout.alignment: Qt.AlignHCenter
                        }
                    }
                }

                // Humidity
                Rectangle {
                    Layout.fillWidth: true
                    Layout.preferredHeight: 52
                    color: Theme.surface
                    radius: Theme.controlRadius

                    ColumnLayout {
                        anchors.centerIn: parent
                        spacing: 2

                        UiText {
                            text: "Humidity"
                            color: Theme.textMuted
                            font.pixelSize: Theme.tinyFontSize
                            Layout.alignment: Qt.AlignHCenter
                        }

                        UiText {
                            text: root.weatherModel.humidity + "%"
                            color: Theme.textStrong
                            font.bold: true
                            font.pixelSize: Theme.fontCaptionSize
                            Layout.alignment: Qt.AlignHCenter
                        }
                    }
                }

                // Wind
                Rectangle {
                    Layout.fillWidth: true
                    Layout.preferredHeight: 52
                    color: Theme.surface
                    radius: Theme.controlRadius

                    ColumnLayout {
                        anchors.centerIn: parent
                        spacing: 2

                        UiText {
                            text: "Wind"
                            color: Theme.textMuted
                            font.pixelSize: Theme.tinyFontSize
                            Layout.alignment: Qt.AlignHCenter
                        }

                        UiText {
                            text: root.weatherModel.wind + " km/h"
                                + (root.weatherModel.windDir ? " " + root.weatherModel.windDir : "")
                            color: Theme.textStrong
                            font.bold: true
                            font.pixelSize: Theme.fontCaptionSize
                            Layout.alignment: Qt.AlignHCenter
                        }
                    }
                }
            }

            // Details Grid Row 2: Pressure, Visibility, Clouds
            RowLayout {
                Layout.fillWidth: true
                spacing: Theme.spacingSm

                // Pressure
                Rectangle {
                    Layout.fillWidth: true
                    Layout.preferredHeight: 52
                    color: Theme.surface
                    radius: Theme.controlRadius

                    ColumnLayout {
                        anchors.centerIn: parent
                        spacing: 2

                        UiText {
                            text: "Pressure"
                            color: Theme.textMuted
                            font.pixelSize: Theme.tinyFontSize
                            Layout.alignment: Qt.AlignHCenter
                        }

                        UiText {
                            text: root.weatherModel.pressure + " hPa"
                            color: Theme.textStrong
                            font.bold: true
                            font.pixelSize: Theme.fontCaptionSize
                            Layout.alignment: Qt.AlignHCenter
                        }
                    }
                }

                // Visibility
                Rectangle {
                    Layout.fillWidth: true
                    Layout.preferredHeight: 52
                    color: Theme.surface
                    radius: Theme.controlRadius

                    ColumnLayout {
                        anchors.centerIn: parent
                        spacing: 2

                        UiText {
                            text: "Visibility"
                            color: Theme.textMuted
                            font.pixelSize: Theme.tinyFontSize
                            Layout.alignment: Qt.AlignHCenter
                        }

                        UiText {
                            text: root.weatherModel.visibility + " km"
                            color: Theme.textStrong
                            font.bold: true
                            font.pixelSize: Theme.fontCaptionSize
                            Layout.alignment: Qt.AlignHCenter
                        }
                    }
                }

                // Clouds
                Rectangle {
                    Layout.fillWidth: true
                    Layout.preferredHeight: 52
                    color: Theme.surface
                    radius: Theme.controlRadius

                    ColumnLayout {
                        anchors.centerIn: parent
                        spacing: 2

                        UiText {
                            text: "Clouds"
                            color: Theme.textMuted
                            font.pixelSize: Theme.tinyFontSize
                            Layout.alignment: Qt.AlignHCenter
                        }

                        UiText {
                            text: root.weatherModel.clouds + "%"
                            color: Theme.textStrong
                            font.bold: true
                            font.pixelSize: Theme.fontCaptionSize
                            Layout.alignment: Qt.AlignHCenter
                        }
                    }
                }
            }

            // Wind Gust row (only shown when gust data available)
            Rectangle {
                Layout.fillWidth: true
                Layout.preferredHeight: 36
                color: Theme.surface
                radius: Theme.controlRadius
                visible: root.weatherModel.windGust !== ""

                RowLayout {
                    anchors.centerIn: parent
                    spacing: Theme.spacingSm

                    UiText {
                        text: "Wind Gust"
                        color: Theme.textMuted
                        font.pixelSize: Theme.tinyFontSize
                    }

                    UiText {
                        text: root.weatherModel.windGust + " km/h"
                        color: Theme.textStrong
                        font.bold: true
                        font.pixelSize: Theme.fontCaptionSize
                    }
                }
            }

            Rectangle {
                Layout.fillWidth: true
                height: 1
                color: Theme.border
            }

            // Sunrise / Sunset row
            RowLayout {
                Layout.fillWidth: true
                spacing: Theme.spacingSm

                // Sunrise
                Rectangle {
                    Layout.fillWidth: true
                    Layout.preferredHeight: 44
                    color: Theme.surface
                    radius: Theme.controlRadius

                    RowLayout {
                        anchors.centerIn: parent
                        spacing: Theme.spacingSm

                        IconText {
                            text: "󰖜"
                            color: Theme.warning
                            font.pixelSize: Theme.scaledFontSize(16, 8)
                        }

                        ColumnLayout {
                            spacing: 0

                            UiText {
                                text: "Sunrise"
                                color: Theme.textMuted
                                font.pixelSize: Theme.tinyFontSize
                            }

                            UiText {
                                text: root.weatherModel.sunrise
                                color: Theme.textStrong
                                font.bold: true
                                font.pixelSize: Theme.fontCaptionSize
                            }
                        }
                    }
                }

                // Sunset
                Rectangle {
                    Layout.fillWidth: true
                    Layout.preferredHeight: 44
                    color: Theme.surface
                    radius: Theme.controlRadius

                    RowLayout {
                        anchors.centerIn: parent
                        spacing: Theme.spacingSm

                        IconText {
                            text: "󰖛"
                            color: Theme.danger
                            font.pixelSize: Theme.scaledFontSize(16, 8)
                        }

                        ColumnLayout {
                            spacing: 0

                            UiText {
                                text: "Sunset"
                                color: Theme.textMuted
                                font.pixelSize: Theme.tinyFontSize
                            }

                            UiText {
                                text: root.weatherModel.sunset
                                color: Theme.textStrong
                                font.bold: true
                                font.pixelSize: Theme.fontCaptionSize
                            }
                        }
                    }
                }
            }
        }
    }
}
