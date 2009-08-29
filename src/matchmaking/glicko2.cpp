/* 
 * File:   glicko2.cpp
 * Author: Catherine
 *
 * Created on August 28, 2009, 1:05 PM
 *
 * This file is a part of Shoddy Battle.
 * Copyright (C) 2009  Catherine Fitzpatrick and Benjamin Gwin
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Affero General Public License
 * as published by the Free Software Foundation; either version 3
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Affero General Public License for more details.
 *
 * You should have received a copy of the GNU Affero General Public License
 * along with this program; if not, visit the Free Software Foundation, Inc.
 * online at http://gnu.org.
 */

#include "glicko2.h"
#include <cmath>

using namespace std;

namespace shoddybattle { namespace glicko2 {

const double PI = 4 * atan(1);

/**
 * Factor used for conversion between the Glicko scale (which is used
 * for presentation) and the Glicko2 scale (which is used for the
 * actual calculations).
 */
const double GLICKO_FACTOR = 173.7178;

inline double getGlicko2Rating(double rating) {
    return (rating - 1500.0) / GLICKO_FACTOR;
}

inline double getGlicko2Deviation(double deviation) {
    return deviation / GLICKO_FACTOR;
}

inline double getGlickoRating(double rating) {
    return GLICKO_FACTOR * rating + 1500.0;
}

inline double getGlickoDeviation(double deviation) {
    return GLICKO_FACTOR * deviation;
}

/**
 * Returns 1/sqrt(1 + (3 * deviation ^ 2) / pi ^ 2)
 */
inline double getGFactor(double deviation) {
    return 1.0 / sqrt(1.0 + (3.0 * deviation * deviation) / (PI * PI));
}

/**
 * Returns 1 / (1 + exp(-g * (rating - opponentRating)))
 * @param g == getGFactor(opponentDevation)
 */
inline double getEFactor(
        double rating,
        double opponentRating, double g) {
    return 1.0 / (1.0 + exp(-g * (rating - opponentRating)));
}

/**
 * Returns (g ^ 2) * e * (1 - e)
 */
inline double getEstimatedVarianceTerm(
        double opponentG, double e) {
    return opponentG * opponentG * e * (1 - e);
}

double getEstimatedVariance(
        const double *opponentG, const double *e, int length) {
    double sum = 0.0;
    for (int i = 0; i < length; ++i) {
        sum += getEstimatedVarianceTerm(opponentG[i], e[i]);
    }
    return 1.0 / sum;
}

double getImprovementSum(
        const double *opponentG,
        const double *e,
        const int *score,
        int length) {
    double sum = 0.0;
    for (int i = 0; i < length; ++i) {
        sum += opponentG[i] * (((double)score[i]) - e[i]);
    }
    return sum;
}

double getUpdatedVolatility(
        double volatility, double deviation, double variance,
        double improvementSum, double system) {
    double improvement = improvementSum * variance;
    double a = log(volatility * volatility);
    double squSystem = system * system;
    double squDeviation = deviation * deviation;
    double x0 = a;
    while (true) {
        double e = exp(x0);
        double d = squDeviation + variance + e;
        double squD = d * d;
        double i = improvement / d;
        double h1 = -(x0 - a) / squSystem - 0.5 * e * i * i;
        double h2 = -1.0 / squSystem
            - 0.5 * e * (squDeviation + variance) / squD
            + 0.5 * variance * variance * e
                * (squDeviation + variance - e) / (squD * d);
        double x1 = x0;
        x0 -= h1 / h2;
        if (abs(x1 - x0) < 0.000001)
            break;
    }
    return exp(x0 / 2.0);
}

inline double getUpdatedDeviation(
        double deviation, double updatedVolatility, double variance) {
    double term = deviation * deviation
        + updatedVolatility * updatedVolatility;
    return 1.0 / sqrt(1.0 / term + 1.0 / variance);
}

inline double getUpdatedRating(
        double rating, double updatedDeviation, double improvementSum) {
    return rating + updatedDeviation * updatedDeviation * improvementSum;
}

/**
 * Find a player's new score, deviation, and volatility at the end of
 * the rating period.
 */
void updatePlayer(PLAYER &player,
        const vector<MATCH> &matches,
        const double system) {
    double volatility = player.volatility;
    double deviation = getGlicko2Deviation(player.deviation);

    const int length = matches.size();
    if (length == 0) {
        player.deviation = getGlickoDeviation(sqrt(
                deviation * deviation + volatility * volatility));
        return;
    }

    double rating = getGlicko2Rating(player.rating);

    double g[length];
    double e[length];
    int score[length];
    for (int i = 0; i < length; ++i) {
        const MATCH &match = matches[i];
        g[i] = getGFactor(getGlicko2Deviation(match.opponentDeviation));
        e[i] = getEFactor(rating, getGlicko2Rating(match.opponentRating), g[i]);
        score[i] = match.score;
    }

    const double variance = getEstimatedVariance(g, e, length);
    const double sum = getImprovementSum(g, e, score, length);

    volatility = getUpdatedVolatility(
        volatility, deviation, variance, sum, system);
    deviation = getUpdatedDeviation(deviation, volatility, variance);
    rating = getUpdatedRating(rating, deviation, sum);

    player.rating = getGlickoRating(rating);
    player.deviation = getGlickoDeviation(deviation);
    player.volatility = volatility;
}

/**
 * Get a rating estimate based on a mean rating and rating deviation using
 * the GLIXARE approach (i.e. returns the chance of a player with the given
 * stats winning against a player who has just joined the ladder).
 */
double getRatingEstimate(const double rating, const double deviation) {
    if (deviation > 100)
        return 0;
    double rds = deviation * deviation;
    double sqr = sqrt(15.905694331435 * (rds + 221781.21786254));
    double inner = (1500.0 - rating) * PI / sqr;
    return floor(10000.0 / (1.0 + pow(10.0, inner)) + 0.5) / 100.0;
}

}} // namespace shoddybattle::glicko2

#if 0

#include <iostream>

int main() {
    using namespace shoddybattle::glicko2;
    cout << getRatingEstimate(1918.558883, 74.43201955) << endl;
}

#endif
