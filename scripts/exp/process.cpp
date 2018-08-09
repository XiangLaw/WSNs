//
// Created by atom on 5/5/18.
//
#include <stdio.h>

int main() {
    FILE *fp = fopen("PacketOverhead.tr", "r");
    char *s;
    fscanf(fp, "%s\t%s\t%s\t%s\t%s\n", s, s, s, s, s);

    long setup_overhead_ = 0, data_fwd_overhead = 0;
    int a, b, c, d, e;
    
    double BI_denominator_ = 0;

    int num_node_ = 0;

    while (!feof(fp)) {
        fscanf(fp, "%d\t%d\t%d\t%d\t%d\n", &a, &b, &c, &d, &e);
        setup_overhead_ += a + b + c + d;
        data_fwd_overhead += e;
        BI_denominator_ += e * e;
        num_node_ += 1;
    }


//    printf("Setup overhead: %ld\nData forwarding overhead: %ld\n", setup_overhead_, data_fwd_overhead);
//    printf("BI: %g\n", (double)(data_fwd_overhead * data_fwd_overhead) / (num_node_ * BI_denominator_));

    FILE *fp1 = fopen("Dijkstra.tr", "r");
    FILE *fp2 = fopen("HopCount.tr", "r");
//
//    int hopcount[10000];
//    double stretch[10000];
//    int dijkstra[10000];
//    int pkt_num_[10000];
//
//    int s_id_, d_id_, hop_count_;
//    for (int i = 0; i < 10000; ++i)
//     {
//     	hopcount[i] = 0;
//     	dijkstra[i] = 0;
//     	pkt_num_[i] = 0;
//     }
//
//    while (!feof(fp2)) {
//    	fscanf(fp2, "%d\t%d\n", &s_id_, &hop_count_);
//    	hopcount[s_id_] += hop_count_;
//    	pkt_num_[s_id_] += 1;
//    }
//
//    while (!feof(fp1)) {
//    	fscanf(fp1, "%d\t%d\t%d\t%s\n", &s_id_, &d_id_, &dijkstra[s_id_]);
//    }
//
//    double max_stretch = 100;
//
//    double sum_stretch_ = 0, num_source_ = 0;
//
//    for (int i = 0; i < 10000; dijkstra[i] != 0) {
//    	stretch[i] = hopcount[i]/(pkt_num_[i] * dijkstra[i]);
//    	max_stretch = stretch[i] < max_stretch ? max_stretch : stretch[i];
//    	sum_stretch_ += stretch[i];
//    	num_source_ += 1;
//    }
//
//    printf("Average stretch: %g\nMax stretch: %g\n", (double)(sum_stretch_/num_source_), max_stretch);

    fclose(fp);
    fclose(fp1);
    fclose(fp2);
    return 0;
}