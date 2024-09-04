/* Webcamoid, webcam capture application.
 * Copyright (C) 2024  Gonzalo Exequiel Pedone
 *
 * Webcamoid is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * Webcamoid is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Webcamoid. If not, see <http://www.gnu.org/licenses/>.
 *
 * Web-Site: http://webcamoid.github.io/
 */

package org.webcamoid.webcamoidutils;

import java.util.Date;
import java.util.HashMap;
import java.util.Map;
import android.app.Activity;
import android.content.res.Configuration;
import android.util.Log;
import android.view.Gravity;
import android.view.ViewTreeObserver;
import android.view.WindowMetrics;
import android.widget.FrameLayout;
import com.google.android.gms.ads.AdError;
import com.google.android.gms.ads.AdRequest;
import com.google.android.gms.ads.AdSize;
import com.google.android.gms.ads.AdView;
import com.google.android.gms.ads.FullScreenContentCallback;
import com.google.android.gms.ads.LoadAdError;
import com.google.android.gms.ads.MobileAds;
import com.google.android.gms.ads.OnUserEarnedRewardListener;
import com.google.android.gms.ads.appopen.AppOpenAd;
import com.google.android.gms.ads.initialization.InitializationStatus;
import com.google.android.gms.ads.initialization.OnInitializationCompleteListener;
import com.google.android.gms.ads.interstitial.InterstitialAd;
import com.google.android.gms.ads.interstitial.InterstitialAdLoadCallback;
import com.google.android.gms.ads.rewarded.RewardItem;
import com.google.android.gms.ads.rewarded.RewardedAd;
import com.google.android.gms.ads.rewarded.RewardedAdLoadCallback;
import com.google.android.gms.ads.rewardedinterstitial.RewardedInterstitialAd;
import com.google.android.gms.ads.rewardedinterstitial.RewardedInterstitialAdLoadCallback;

public class AdManager extends FullScreenContentCallback
{
    enum AdType
    {
        Banner,
        AdaptiveBanner,
        AppOpen,
        Interstitial,
        Rewarded,
        RewardedInterstitial
    }

    enum AdCompletionStatus
    {
        Showed,
        Clicked,
        Impressed,
        Dismissed,
        Failed
    }

    // Public API

    public AdManager(Activity activity, Map<AdType, String> adUnitIDMap)
    {
        this.activity = activity;
        this.adUnitIDMap = adUnitIDMap;
        this.adsMap = new HashMap<AdType, Object>();
    }

    public boolean initialize()
    {
        MobileAds.initialize(this.activity,
                             new OnInitializationCompleteListener() {
            @Override
            public void onInitializationComplete(InitializationStatus initializationStatus) {
            }
        });

        /* FIXME: This probably will fail since there is not any view defined
         * for showing the ads yet.
         */
        this.adContainerView = this.activity
                                   .getWindow()
                                   .getDecorView()
                                   .findViewById(android.R.id.content);

        if (this.adContainerView == null)
            return false;

        this.adContainerView.getViewTreeObserver()
                            .addOnGlobalLayoutListener(new ViewTreeObserver.OnGlobalLayoutListener() {
            @Override
            public void onGlobalLayout() {
                if (!isInitializedOnce) {
                    isInitializedOnce = true;
                    initialized();
                }
                if (!isInitialized) {
                    isInitialized = true;
                    layoutChanged();
                }
            }
        });

        return true;
    }

    public boolean show(AdType type)
    {
        switch (type) {
        case Banner:
        case AdaptiveBanner:
            return this.showBanner(type);

        case AppOpen:
            return this.showAppOpen();

        case Interstitial:
            return this.showInterstitial();

        case Rewarded:
            return this.showRewarded();

        case RewardedInterstitial:
            return this.showRewardedInterstitial();

        default:
            break;
        }

        return false;
    }

    public AdSize bannerSize()
    {
        float adWidthPixels = this.adContainerView.getWidth();

        if (adWidthPixels == 0f)
            adWidthPixels = this.activity
                                .getWindow()
                                .getDecorView()
                                .getWidth();

        float density = this.activity
                            .getResources()
                            .getDisplayMetrics()
                            .density;
        int adWidth = (int) (adWidthPixels / density);

        return AdSize.getCurrentOrientationAnchoredAdaptiveBannerAdSize(this.activity,
                                                                        adWidth);
    }

    // Callbacks

    private void initialized()
    {
        this.show(AdType.AppOpen);
        //this.load(AdType.Interstitial);
        //this.load(AdType.Rewarded);
        //this.load(AdType.RewardedInterstitial);
    }

    private void layoutChanged()
    {
        this.show(AdType.AdaptiveBanner);
    }

    private void loaded(AdType type)
    {
    }

    private void completed(AdType type, AdCompletionStatus status)
    {
    }

    private void rewardEarned(int amount, String type)
    {
    }

    public void bannerSizeChanged(AdSize size)
    {
    }

    // Private API

    private Activity activity;
    private Map<AdType, String> adUnitIDMap;
    private Map<AdType, Object> adsMap;
    private FrameLayout adContainerView;
    private boolean isInitialized = false;
    private static boolean isInitializedOnce = false;
    private boolean isAdLoading = false;
    private boolean isAdShowing = false;
    private long loadTime = 0;

    private boolean isAdAvailable(AdType type)
    {
        return this.adsMap.get(type) != null;
    }

    private boolean load(AdType type)
    {
        return this.load(type, false);
    }

    private boolean load(AdType type, boolean showAd)
    {
        switch (type) {
        case Banner:
        case AdaptiveBanner:
            return this.loadBanner(type, showAd);

        case AppOpen:
            return this.loadAppOpen(showAd);

        case Interstitial:
            return this.loadInterstitial(showAd);

        case Rewarded:
            return this.loadRewarded(showAd);

        case RewardedInterstitial:
            return this.loadRewardedInterstitial(showAd);

        default:
            break;
        }

        return false;
    }

    private boolean loadBanner(AdType type, boolean showAd)
    {
        AdView ad = new AdView(this.activity);

        if (ad == null)
            return false;

        this.adsMap.put(type, ad);
        ad.setLayoutParams(new FrameLayout.LayoutParams(
                           FrameLayout.LayoutParams.WRAP_CONTENT,
                           FrameLayout.LayoutParams.WRAP_CONTENT,
                           Gravity.CENTER_HORIZONTAL | Gravity.BOTTOM));
        this.adContainerView.addView(ad);
        this.loaded(type);

        if (showAd)
            this.show(type);

        return true;
    }

    private boolean loadAppOpen(final boolean showAd)
    {
        final AdType type = AdType.AppOpen;

        if (this.isAdLoading || this.isAdAvailable(type))
            return false;

        if (!this.adUnitIDMap.containsKey(type))
            return false;

        this.isAdLoading = true;
        AppOpenAd.load(this.activity,
                       this.adUnitIDMap.get(type),
                       new AdRequest.Builder().build(),
                       this.activity.getResources().getConfiguration().orientation == Configuration.ORIENTATION_PORTRAIT?
                           AppOpenAd.APP_OPEN_AD_ORIENTATION_PORTRAIT:
                           AppOpenAd.APP_OPEN_AD_ORIENTATION_LANDSCAPE,
                       new AppOpenAd.AppOpenAdLoadCallback() {
                           @Override
                           public void onAdLoaded(AppOpenAd ad) {
                               adsMap.put(type, ad);
                               isAdLoading = false;
                               loadTime = (new Date()).getTime();
                               loaded(type);

                               if (showAd)
                                   show(type);
                           }

                           @Override
                           public void onAdFailedToLoad(LoadAdError loadAdError) {
                               adsMap.put(type, null);
                               isAdLoading = false;
                           }
                       });

        return true;
    }

    private boolean loadInterstitial(final boolean showAd)
    {
        final AdType type = AdType.Interstitial;

        if (this.isAdLoading || this.isAdAvailable(type))
            return false;

        if (!this.adUnitIDMap.containsKey(type))
            return false;

        this.isAdLoading = true;
        InterstitialAd.load(this.activity,
                            this.adUnitIDMap.get(type),
                            new AdRequest.Builder().build(),
                            new InterstitialAdLoadCallback() {
                                @Override
                                public void onAdLoaded(InterstitialAd ad) {
                                    adsMap.put(type, ad);
                                    isAdLoading = false;
                                    loadTime = (new Date()).getTime();
                                    loaded(type);

                                    if (showAd)
                                        show(type);
                                }

                                @Override
                                public void onAdFailedToLoad(LoadAdError loadAdError) {
                                    adsMap.put(type, null);
                                    isAdLoading = false;
                                }
                            });

        return true;
    }

    private boolean loadRewarded(final boolean showAd)
    {
        final AdType type = AdType.Rewarded;

        if (this.isAdLoading || this.isAdAvailable(type))
            return false;

        if (!this.adUnitIDMap.containsKey(type))
            return false;

        this.isAdLoading = true;
        RewardedAd.load(this.activity,
                        this.adUnitIDMap.get(type),
                        new AdRequest.Builder().build(),
                        new RewardedAdLoadCallback() {
                            @Override
                            public void onAdLoaded(RewardedAd ad) {
                                adsMap.put(type, ad);
                                isAdLoading = false;
                                loadTime = (new Date()).getTime();
                                loaded(type);

                                if (showAd)
                                    show(type);
                            }

                            @Override
                            public void onAdFailedToLoad(LoadAdError loadAdError) {
                                adsMap.put(type, null);
                                isAdLoading = false;
                            }
                        });

        return true;
    }

    private boolean loadRewardedInterstitial(final boolean showAd)
    {
        final AdType type = AdType.RewardedInterstitial;

        if (this.isAdLoading || this.isAdAvailable(type))
            return false;

        if (!this.adUnitIDMap.containsKey(type))
            return false;

        this.isAdLoading = true;
        RewardedInterstitialAd.load(this.activity,
                                    this.adUnitIDMap.get(type),
                                    new AdRequest.Builder().build(),
                                    new RewardedInterstitialAdLoadCallback() {
                                        @Override
                                        public void onAdLoaded(RewardedInterstitialAd ad) {
                                            adsMap.put(type, ad);
                                            isAdLoading = false;
                                            loadTime = (new Date()).getTime();
                                            loaded(type);

                                            if (showAd)
                                                show(type);
                                        }

                                        @Override
                                        public void onAdFailedToLoad(LoadAdError loadAdError) {
                                            adsMap.put(type, null);
                                            isAdLoading = false;
                                        }
                                     });

        return true;
    }

    private boolean showBanner(AdType type)
    {
        if (!this.adUnitIDMap.containsKey(type))
            return false;

        if (!this.adsMap.containsKey(type)) {
            this.completed(type, AdCompletionStatus.Failed);
            this.load(type, true);

            return false;
        }

        AdView ad = (AdView) this.adsMap.get(type);
        ad.setAdUnitId(this.adUnitIDMap.get(type));
        AdSize size = this.bannerSize();
        ad.setAdSize(size);
        ad.loadAd(new AdRequest.Builder().build());
        this.bannerSizeChanged(size);
        this.completed(type, AdCompletionStatus.Showed);

        return true;
    }

    private boolean showAppOpen()
    {
        final AdType type = AdType.AppOpen;

        if (this.isAdShowing)
            return false;

        if (!this.isAdAvailable(type)) {
            this.completed(type, AdCompletionStatus.Failed);
            this.load(type, true);

            return false;
        }

        AppOpenAd ad = (AppOpenAd) adsMap.get(type);
        ad.setFullScreenContentCallback(new HandleFullScreen(this, type));
        this.isAdShowing = true;
        ad.show(this.activity);

        return true;
    }

    private boolean showInterstitial()
    {
        final AdType type = AdType.Interstitial;

        if (this.isAdShowing)
            return false;

        if (!this.isAdAvailable(type)) {
            this.completed(type, AdCompletionStatus.Failed);
            this.load(type, true);

            return false;
        }

        InterstitialAd ad = (InterstitialAd) adsMap.get(type);
        ad.setFullScreenContentCallback(new HandleFullScreen(this, type));
        this.isAdShowing = true;
        ad.show(this.activity);

        return true;
    }

    private boolean showRewarded()
    {
        final AdType type = AdType.Rewarded;

        if (this.isAdShowing)
            return false;

        if (!this.isAdAvailable(type)) {
            this.completed(type, AdCompletionStatus.Failed);
            this.load(type, true);

            return false;
        }

        RewardedAd ad = (RewardedAd) adsMap.get(type);
        ad.setFullScreenContentCallback(new HandleFullScreen(this, type));
        this.isAdShowing = true;
        ad.show(this.activity, new HandleReward(this));

        return true;
    }

    private boolean showRewardedInterstitial()
    {
        final AdType type = AdType.RewardedInterstitial;

        if (this.isAdShowing)
            return false;

        if (!this.isAdAvailable(type)) {
            this.completed(type, AdCompletionStatus.Failed);
            this.load(type, true);

            return false;
        }

        RewardedInterstitialAd ad = (RewardedInterstitialAd) adsMap.get(type);
        ad.setFullScreenContentCallback(new HandleFullScreen(this, type));
        this.isAdShowing = true;
        ad.show(this.activity, new HandleReward(this));

        return true;
    }

    private class HandleFullScreen extends FullScreenContentCallback
    {
        private AdManager manager;
        private AdType type;

        public HandleFullScreen(AdManager manager, AdType type)
        {
            this.manager = manager;
            this.type = type;
        }

        @Override
        public void onAdShowedFullScreenContent()
        {
            this.manager.completed(this.type, AdCompletionStatus.Showed);
        }

        @Override
        public void onAdClicked()
        {
            this.manager.completed(this.type, AdCompletionStatus.Clicked);
        }

        @Override
        public void onAdImpression()
        {
            this.manager.completed(this.type, AdCompletionStatus.Impressed);
        }

        @Override
        public void onAdDismissedFullScreenContent()
        {
            this.manager.adsMap.put(this.type, null);
            this.manager.isAdShowing = false;
            this.manager.completed(this.type, AdCompletionStatus.Dismissed);
            this.manager.load(this.type);
        }

        @Override
        public void onAdFailedToShowFullScreenContent(AdError adError)
        {
            this.manager.adsMap.put(this.type, null);
            this.manager.isAdShowing = false;
            this.manager.completed(this.type, AdCompletionStatus.Failed);
            this.manager.load(this.type);
        }
    }

    private class HandleReward implements OnUserEarnedRewardListener
    {
        private AdManager manager;

        public HandleReward(AdManager manager)
        {
            this.manager = manager;
        }

        @Override
        public void onUserEarnedReward(RewardItem rewardItem)
        {
            this.manager.rewardEarned(rewardItem.getAmount(),
                                      rewardItem.getType());
        }
    }
}
