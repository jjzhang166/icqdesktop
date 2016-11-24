#include "stdafx.h"

#include "translit.h"

namespace
{
    QMap<QChar, QStringList> makeTable()
    {
        QMap<QChar, QStringList> result;
        
        result[QChar(1040)].append(QChar(97));
        result[QChar(1041)].append(QChar(98));
        result[QChar(1042)].append(QChar(118));
        result[QChar(1042)].append(QChar(119));
        result[QChar(1043)].append(QChar(103));
        result[QChar(1044)].append(QChar(100));
        result[QChar(1045)].append(QChar(101));
        result[QChar(1045)].append(QString(QChar(121)) + QChar(101));
        result[QChar(1025)].append(QString(QChar(121)) + QChar(111));
        result[QChar(1025)].append(QString(QChar(105)) + QChar(111));
        result[QChar(1025)].append(QString(QChar(106)) + QChar(111));
        result[QChar(1025)].append(QChar(101));
        result[QChar(1046)].append(QChar(106));
        result[QChar(1046)].append(QChar(122));
        result[QChar(1046)].append(QString(QChar(122)) + QChar(104));
        result[QChar(1046)].append(QChar(103));
        result[QChar(1047)].append(QChar(122));
        result[QChar(1047)].append(QChar(115));
        result[QChar(1048)].append(QChar(105));
        result[QChar(1048)].append(QChar(121));
        result[QChar(1049)].append(QChar(105));
        result[QChar(1049)].append(QChar(121));
        result[QChar(1050)].append(QChar(107));
        result[QChar(1050)].append(QString(QChar(99)) + QChar(107));
        result[QChar(1050)].append(QChar(99));
        result[QChar(1051)].append(QChar(108));
        result[QChar(1052)].append(QChar(109));
        result[QChar(1053)].append(QChar(110));
        result[QChar(1054)].append(QChar(111));
        result[QChar(1055)].append(QChar(112));
        result[QChar(1056)].append(QChar(114));
        result[QChar(1057)].append(QChar(115));
        result[QChar(1057)].append(QChar(99));
        result[QChar(1058)].append(QChar(116));
        result[QChar(1059)].append(QChar(117));
        result[QChar(1059)].append(QChar(121));
        result[QChar(1060)].append(QChar(102));
        result[QChar(1060)].append(QString(QChar(112)) + QChar(104));
        result[QChar(1061)].append(QChar(104));
        result[QChar(1061)].append(QString(QChar(107)) + QChar(104));
        result[QChar(1062)].append(QChar(99));
        result[QChar(1062)].append(QString(QChar(116)) + QChar(115));
        result[QChar(1063)].append(QString(QChar(99)) + QChar(104));
        result[QChar(1064)].append(QString(QChar(115)) + QChar(104));
        result[QChar(1064)].append(QString(QChar(115)) + QChar(99) + QChar(104));
        result[QChar(1065)].append(QString(QChar(115)) + QChar(99) + QChar(104));
        result[QChar(1065)].append(QString(QChar(115)) + QChar(104) + QChar(99) + QChar(104));
        result[QChar(1066)].append(QChar(39));
        result[QChar(1066)].append(QChar(105));
        result[QChar(1066)].append(QChar(106));
        result[QChar(1066)].append(QChar(121));
        result[QChar(1067)].append(QChar(105));
        result[QChar(1067)].append(QChar(121));
        result[QChar(1068)].append(QChar(105));
        result[QChar(1068)].append(QChar(106));
        result[QChar(1068)].append(QChar(121));
        result[QChar(1068)].append(QString());
        result[QChar(1069)].append(QChar(101));
        result[QChar(1070)].append(QChar(117));
        result[QChar(1070)].append(QString(QChar(106)) + QChar(117));
        result[QChar(1070)].append(QString(QChar(121)) + QChar(117));
        result[QChar(1071)].append(QString(QChar(105)) + QChar(97));
        result[QChar(1071)].append(QString(QChar(106)) + QChar(97));
        result[QChar(1071)].append(QString(QChar(121)) + QChar(97));

        result[QChar(65)].append(QChar(1040));
        result[QChar(66)].append(QChar(1041));
        result[QChar(67)].append(QChar(1062));
        result[QChar(67)].append(QChar(1050));
        result[QChar(67)].append(QChar(1057));
        result[QChar(68)].append(QChar(1044));
        result[QChar(69)].append(QChar(1045));
        result[QChar(69)].append(QChar(1069));
        result[QChar(69)].append(QChar(1025));
        result[QChar(70)].append(QChar(1060));
        result[QChar(71)].append(QChar(1043));
        result[QChar(71)].append(QString(QChar(1044)) + QChar(1046));
        result[QChar(72)].append(QChar(1061));
        result[QChar(73)].append(QChar(1048));
        result[QChar(73)].append(QChar(1067));
        result[QChar(73)].append(QChar(1068));
        result[QChar(73)].append(QChar(1066));
        result[QChar(73)].append(QChar(1049));
        result[QChar(74)].append(QChar(1046));
        result[QChar(74)].append(QChar(1066));
        result[QChar(74)].append(QChar(1068));
        result[QChar(74)].append(QString(QChar(1044)) + QChar(1046));
        result[QChar(75)].append(QChar(1050));
        result[QChar(76)].append(QChar(1051));
        result[QChar(77)].append(QChar(1052));
        result[QChar(78)].append(QChar(1053));
        result[QChar(79)].append(QChar(1054));
        result[QChar(80)].append(QChar(1055));
        result[QChar(81)].append(QChar(1050));
        result[QChar(82)].append(QChar(1056));
        result[QChar(83)].append(QChar(1057));
        result[QChar(84)].append(QChar(1058));
        result[QChar(85)].append(QChar(1059));
        result[QChar(85)].append(QChar(1070));
        result[QChar(86)].append(QChar(1042));
        result[QChar(87)].append(QChar(1042));
        result[QChar(88)].append(QString(QChar(1050)) + QChar(1057));
        result[QChar(88)].append(QString(QChar(1048)) + QChar(1050) + QChar(1057));
        result[QChar(89)].append(QChar(1049));
        result[QChar(89)].append(QChar(1059));
        result[QChar(89)].append(QChar(1067));
        result[QChar(89)].append(QChar(1048));
        result[QChar(89)].append(QChar(1066));
        result[QChar(89)].append(QChar(1068));
        result[QChar(90)].append(QChar(1047));
        result[QChar(90)].append(QChar(1046));

        result[QChar(32)].append(QChar(32));
        result[QChar(39)].append(QChar(1068));
        result[QChar(39)].append(QChar(1066));


        return result;
    }
}

namespace Translit
{
    std::vector<QStringList> getPossibleStrings(const QString& text)
    {
        static QMap<QChar, QStringList> table = makeTable();

        int max = std::min(getMaxSearchTextLength(), text.length());
        std::vector<QStringList> result(max);

        for (auto i = 0; i < max; ++i)
        {
            auto chList = table[text.at(i).toUpper()];
            if (chList.empty())
            {
                return std::vector<QStringList>();
            }

            for (int j = 0; j < chList.size(); ++j)
                result[i].append(chList[j]);
        }

        return result;
    }

    int getMaxSearchTextLength()
    {
        return 50;
    }
}
