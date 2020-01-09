// Copyright (C) 2019 Google LLC
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//      http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include "icing/index/iterator/doc-hit-info-iterator-and.h"

#include <string>
#include <vector>

#include "gmock/gmock.h"
#include "gtest/gtest.h"
#include "icing/index/hit/doc-hit-info.h"
#include "icing/index/iterator/doc-hit-info-iterator-test-util.h"
#include "icing/index/iterator/doc-hit-info-iterator.h"
#include "icing/schema/section.h"
#include "icing/store/document-id.h"
#include "icing/testing/common-matchers.h"

namespace icing {
namespace lib {

namespace {

using ::testing::ElementsAre;
using ::testing::Eq;
using ::testing::IsEmpty;

TEST(CreateAndIteratorTest, And) {
  // Basic test that we can create a working And iterator. Further testing of
  // the And iterator should be done separately below.
  std::vector<DocHitInfo> doc_hit_infos = {DocHitInfo(10)};
  std::unique_ptr<DocHitInfoIterator> first_iter =
      std::make_unique<DocHitInfoIteratorDummy>(doc_hit_infos);
  std::unique_ptr<DocHitInfoIterator> second_iter =
      std::make_unique<DocHitInfoIteratorDummy>(doc_hit_infos);

  std::vector<std::unique_ptr<DocHitInfoIterator>> iterators;
  iterators.push_back(std::move(first_iter));
  iterators.push_back(std::move(second_iter));
  std::unique_ptr<DocHitInfoIterator> and_iter =
      CreateAndIterator(std::move(iterators));

  EXPECT_THAT(GetDocumentIds(and_iter.get()), ElementsAre(10));
}

TEST(CreateAndIteratorTest, AndNary) {
  // Basic test that we can create a working AndNary iterator. Further testing
  // of the AndNary iterator should be done separately below.
  std::vector<DocHitInfo> doc_hit_infos = {DocHitInfo(10)};
  std::vector<std::unique_ptr<DocHitInfoIterator>> iterators;
  iterators.push_back(std::make_unique<DocHitInfoIteratorDummy>(doc_hit_infos));
  iterators.push_back(std::make_unique<DocHitInfoIteratorDummy>(doc_hit_infos));
  iterators.push_back(std::make_unique<DocHitInfoIteratorDummy>(doc_hit_infos));
  iterators.push_back(std::make_unique<DocHitInfoIteratorDummy>(doc_hit_infos));

  std::unique_ptr<DocHitInfoIterator> and_iter =
      CreateAndIterator(std::move(iterators));

  EXPECT_THAT(GetDocumentIds(and_iter.get()), ElementsAre(10));
}

TEST(DocHitInfoIteratorAndTest, Initialize) {
  DocHitInfoIteratorAnd and_iter(std::make_unique<DocHitInfoIteratorDummy>(),
                                 std::make_unique<DocHitInfoIteratorDummy>());

  // We start out with invalid values
  EXPECT_THAT(and_iter.doc_hit_info(), Eq(DocHitInfo(kInvalidDocumentId)));
  EXPECT_THAT(and_iter.hit_intersect_section_ids_mask(),
              Eq(kSectionIdMaskNone));
}

TEST(DocHitInfoIteratorAndTest, GetNumBlocksInspected) {
  int first_iter_blocks = 4;  // arbitrary value
  auto first_iter = std::make_unique<DocHitInfoIteratorDummy>();
  first_iter->SetNumBlocksInspected(first_iter_blocks);

  int second_iter_blocks = 7;  // arbitrary value
  auto second_iter = std::make_unique<DocHitInfoIteratorDummy>();
  second_iter->SetNumBlocksInspected(second_iter_blocks);

  DocHitInfoIteratorAnd and_iter(std::move(first_iter), std::move(second_iter));

  EXPECT_THAT(and_iter.GetNumBlocksInspected(),
              Eq(first_iter_blocks + second_iter_blocks));
}

TEST(DocHitInfoIteratorAndTest, GetNumLeafAdvanceCalls) {
  int first_iter_leaves = 4;  // arbitrary value
  auto first_iter = std::make_unique<DocHitInfoIteratorDummy>();
  first_iter->SetNumLeafAdvanceCalls(first_iter_leaves);

  int second_iter_leaves = 7;  // arbitrary value
  auto second_iter = std::make_unique<DocHitInfoIteratorDummy>();
  second_iter->SetNumLeafAdvanceCalls(second_iter_leaves);

  DocHitInfoIteratorAnd and_iter(std::move(first_iter), std::move(second_iter));

  EXPECT_THAT(and_iter.GetNumLeafAdvanceCalls(),
              Eq(first_iter_leaves + second_iter_leaves));
}

TEST(DocHitInfoIteratorAndTest, AdvanceNoOverlap) {
  std::vector<DocHitInfo> first_vector = {DocHitInfo(10), DocHitInfo(9)};

  std::vector<DocHitInfo> second_vector = {DocHitInfo(8), DocHitInfo(7)};

  std::unique_ptr<DocHitInfoIterator> first_iter =
      std::make_unique<DocHitInfoIteratorDummy>(first_vector);
  std::unique_ptr<DocHitInfoIterator> second_iter =
      std::make_unique<DocHitInfoIteratorDummy>(second_vector);
  DocHitInfoIteratorAnd and_iter(std::move(first_iter), std::move(second_iter));

  EXPECT_THAT(GetDocumentIds(&and_iter), IsEmpty());
}

TEST(DocHitInfoIteratorAndTest, Advance) {
  std::vector<DocHitInfo> first_vector = {DocHitInfo(10), DocHitInfo(8),
                                          DocHitInfo(6),  DocHitInfo(4),
                                          DocHitInfo(2),  DocHitInfo(0)};

  std::vector<DocHitInfo> second_vector = {DocHitInfo(8), DocHitInfo(4),
                                           DocHitInfo(0)};

  std::unique_ptr<DocHitInfoIterator> first_iter =
      std::make_unique<DocHitInfoIteratorDummy>(first_vector);
  std::unique_ptr<DocHitInfoIterator> second_iter =
      std::make_unique<DocHitInfoIteratorDummy>(second_vector);
  DocHitInfoIteratorAnd and_iter(std::move(first_iter), std::move(second_iter));

  EXPECT_THAT(GetDocumentIds(&and_iter), ElementsAre(8, 4, 0));
}

TEST(DocHitInfoIteratorAndTest, AdvanceNestedIterators) {
  std::vector<DocHitInfo> first_vector = {
      DocHitInfo(10), DocHitInfo(9), DocHitInfo(8), DocHitInfo(7),
      DocHitInfo(6),  DocHitInfo(5), DocHitInfo(4), DocHitInfo(3),
      DocHitInfo(2),  DocHitInfo(1), DocHitInfo(0)};

  std::vector<DocHitInfo> second_vector = {DocHitInfo(10), DocHitInfo(8),
                                           DocHitInfo(6),  DocHitInfo(4),
                                           DocHitInfo(2),  DocHitInfo(0)};

  std::vector<DocHitInfo> third_vector = {DocHitInfo(10), DocHitInfo(7),
                                          DocHitInfo(6), DocHitInfo(2),
                                          DocHitInfo(1)};

  std::unique_ptr<DocHitInfoIterator> first_iter =
      std::make_unique<DocHitInfoIteratorDummy>(first_vector);
  std::unique_ptr<DocHitInfoIterator> second_iter =
      std::make_unique<DocHitInfoIteratorDummy>(second_vector);
  std::unique_ptr<DocHitInfoIterator> third_iter =
      std::make_unique<DocHitInfoIteratorDummy>(third_vector);

  std::unique_ptr<DocHitInfoIterator> inner_iter =
      std::make_unique<DocHitInfoIteratorAnd>(std::move(first_iter),
                                              std::move(second_iter));
  std::unique_ptr<DocHitInfoIterator> outer_iter =
      std::make_unique<DocHitInfoIteratorAnd>(std::move(inner_iter),
                                              std::move(third_iter));

  EXPECT_THAT(GetDocumentIds(outer_iter.get()), ElementsAre(10, 6, 2));
}

TEST(DocHitInfoIteratorAndTest, SectionIdMask) {
  // Arbitrary section ids for the documents in the DocHitInfoIterators.
  // Created to test correct section_id_mask behavior.
  SectionIdMask section_id_mask1 = 0b01010101;  // hits in sections 0, 2, 4, 6
  SectionIdMask section_id_mask2 = 0b00000110;  // hits in sections 1, 2
  SectionIdMask mask_anded_result = 0b00000100;
  SectionIdMask mask_ored_result = 0b01010111;

  std::vector<DocHitInfo> first_vector = {DocHitInfo(4, section_id_mask1)};
  std::vector<DocHitInfo> second_vector = {DocHitInfo(4, section_id_mask2)};

  auto first_iter = std::make_unique<DocHitInfoIteratorDummy>(first_vector);
  first_iter->set_hit_intersect_section_ids_mask(section_id_mask1);

  auto second_iter = std::make_unique<DocHitInfoIteratorDummy>(second_vector);
  second_iter->set_hit_intersect_section_ids_mask(section_id_mask2);

  DocHitInfoIteratorAnd and_iter(std::move(first_iter), std::move(second_iter));

  ICING_EXPECT_OK(and_iter.Advance());
  EXPECT_THAT(and_iter.doc_hit_info().hit_section_ids_mask(),
              Eq(mask_ored_result));
  EXPECT_THAT(and_iter.hit_intersect_section_ids_mask(), Eq(mask_anded_result));
}

TEST(DocHitInfoIteratorAndNaryTest, Initialize) {
  std::vector<std::unique_ptr<DocHitInfoIterator>> iterators;
  iterators.push_back(std::make_unique<DocHitInfoIteratorDummy>());
  iterators.push_back(std::make_unique<DocHitInfoIteratorDummy>());
  iterators.push_back(std::make_unique<DocHitInfoIteratorDummy>());
  iterators.push_back(std::make_unique<DocHitInfoIteratorDummy>());
  DocHitInfoIteratorAndNary and_iter(std::move(iterators));

  // We start out with invalid values
  EXPECT_THAT(and_iter.doc_hit_info(), Eq(DocHitInfo(kInvalidDocumentId)));
  EXPECT_THAT(and_iter.hit_intersect_section_ids_mask(),
              Eq(kSectionIdMaskNone));
}

TEST(DocHitInfoIteratorAndNaryTest, InitializeEmpty) {
  // We can initialize it fine even with an empty vector
  std::vector<std::unique_ptr<DocHitInfoIterator>> empty_vector;
  DocHitInfoIteratorAndNary empty_iter(std::move(empty_vector));

  // But it won't be able to advance anywhere
  EXPECT_THAT(empty_iter.Advance(),
              StatusIs(libtextclassifier3::StatusCode::INVALID_ARGUMENT));
}

TEST(DocHitInfoIteratorAndNaryTest, GetNumBlocksInspected) {
  int first_iter_blocks = 4;  // arbitrary value
  auto first_iter = std::make_unique<DocHitInfoIteratorDummy>();
  first_iter->SetNumBlocksInspected(first_iter_blocks);

  int second_iter_blocks = 7;  // arbitrary value
  auto second_iter = std::make_unique<DocHitInfoIteratorDummy>();
  second_iter->SetNumBlocksInspected(second_iter_blocks);

  int third_iter_blocks = 13;  // arbitrary value
  auto third_iter = std::make_unique<DocHitInfoIteratorDummy>();
  third_iter->SetNumBlocksInspected(third_iter_blocks);

  int fourth_iter_blocks = 1;  // arbitrary value
  auto fourth_iter = std::make_unique<DocHitInfoIteratorDummy>();
  fourth_iter->SetNumBlocksInspected(fourth_iter_blocks);

  std::vector<std::unique_ptr<DocHitInfoIterator>> iterators;
  iterators.push_back(std::move(first_iter));
  iterators.push_back(std::move(second_iter));
  iterators.push_back(std::move(third_iter));
  iterators.push_back(std::move(fourth_iter));
  DocHitInfoIteratorAndNary and_iter(std::move(iterators));

  EXPECT_THAT(and_iter.GetNumBlocksInspected(),
              Eq(first_iter_blocks + second_iter_blocks + third_iter_blocks +
                 fourth_iter_blocks));
}

TEST(DocHitInfoIteratorAndNaryTest, GetNumLeafAdvanceCalls) {
  int first_iter_leaves = 4;  // arbitrary value
  auto first_iter = std::make_unique<DocHitInfoIteratorDummy>();
  first_iter->SetNumLeafAdvanceCalls(first_iter_leaves);

  int second_iter_leaves = 7;  // arbitrary value
  auto second_iter = std::make_unique<DocHitInfoIteratorDummy>();
  second_iter->SetNumLeafAdvanceCalls(second_iter_leaves);

  int third_iter_leaves = 13;  // arbitrary value
  auto third_iter = std::make_unique<DocHitInfoIteratorDummy>();
  third_iter->SetNumLeafAdvanceCalls(third_iter_leaves);

  int fourth_iter_leaves = 13;  // arbitrary value
  auto fourth_iter = std::make_unique<DocHitInfoIteratorDummy>();
  fourth_iter->SetNumLeafAdvanceCalls(fourth_iter_leaves);

  std::vector<std::unique_ptr<DocHitInfoIterator>> iterators;
  iterators.push_back(std::move(first_iter));
  iterators.push_back(std::move(second_iter));
  iterators.push_back(std::move(third_iter));
  iterators.push_back(std::move(fourth_iter));
  DocHitInfoIteratorAndNary and_iter(std::move(iterators));

  EXPECT_THAT(and_iter.GetNumLeafAdvanceCalls(),
              Eq(first_iter_leaves + second_iter_leaves + third_iter_leaves +
                 fourth_iter_leaves));
}

TEST(DocHitInfoIteratorAndNaryTest, Advance) {
  std::vector<DocHitInfo> first_vector = {
      DocHitInfo(10), DocHitInfo(9), DocHitInfo(8), DocHitInfo(7),
      DocHitInfo(6),  DocHitInfo(5), DocHitInfo(4), DocHitInfo(3),
      DocHitInfo(2),  DocHitInfo(1), DocHitInfo(0)};

  std::vector<DocHitInfo> second_vector = {DocHitInfo(10), DocHitInfo(8),
                                           DocHitInfo(6),  DocHitInfo(4),
                                           DocHitInfo(2),  DocHitInfo(0)};

  std::vector<DocHitInfo> third_vector = {DocHitInfo(9), DocHitInfo(6),
                                          DocHitInfo(3), DocHitInfo(0)};

  std::vector<DocHitInfo> fourth_vector = {DocHitInfo(6), DocHitInfo(5),
                                           DocHitInfo(0)};

  std::vector<std::unique_ptr<DocHitInfoIterator>> iterators;
  iterators.push_back(std::make_unique<DocHitInfoIteratorDummy>(first_vector));
  iterators.push_back(std::make_unique<DocHitInfoIteratorDummy>(second_vector));
  iterators.push_back(std::make_unique<DocHitInfoIteratorDummy>(third_vector));
  iterators.push_back(std::make_unique<DocHitInfoIteratorDummy>(fourth_vector));
  DocHitInfoIteratorAndNary and_iter(std::move(iterators));

  EXPECT_THAT(GetDocumentIds(&and_iter), ElementsAre(6, 0));
}

TEST(DocHitInfoIteratorAndNaryTest, SectionIdMask) {
  // Arbitrary section ids for the documents in the DocHitInfoIterators.
  // Created to test correct section_id_mask behavior.
  SectionIdMask section_id_mask1 = 0b01000101;  // hits in sections 0, 2, 6
  SectionIdMask section_id_mask2 = 0b00000110;  // hits in sections 1, 2
  SectionIdMask section_id_mask3 = 0b00001100;  // hits in sections 2, 3
  SectionIdMask section_id_mask4 = 0b00100100;  // hits in sections 2, 5
  SectionIdMask mask_anded_result = 0b00000100;
  SectionIdMask mask_ored_result = 0b01101111;

  std::vector<DocHitInfo> first_vector = {DocHitInfo(4, section_id_mask1)};
  std::vector<DocHitInfo> second_vector = {DocHitInfo(4, section_id_mask2)};
  std::vector<DocHitInfo> third_vector = {DocHitInfo(4, section_id_mask3)};
  std::vector<DocHitInfo> fourth_vector = {DocHitInfo(4, section_id_mask4)};

  auto first_iter = std::make_unique<DocHitInfoIteratorDummy>(first_vector);
  first_iter->set_hit_intersect_section_ids_mask(section_id_mask1);

  auto second_iter = std::make_unique<DocHitInfoIteratorDummy>(second_vector);
  second_iter->set_hit_intersect_section_ids_mask(section_id_mask2);

  auto third_iter = std::make_unique<DocHitInfoIteratorDummy>(third_vector);
  third_iter->set_hit_intersect_section_ids_mask(section_id_mask3);

  auto fourth_iter = std::make_unique<DocHitInfoIteratorDummy>(fourth_vector);
  fourth_iter->set_hit_intersect_section_ids_mask(section_id_mask4);

  std::vector<std::unique_ptr<DocHitInfoIterator>> iterators;
  iterators.push_back(std::move(first_iter));
  iterators.push_back(std::move(second_iter));
  iterators.push_back(std::move(third_iter));
  iterators.push_back(std::move(fourth_iter));

  DocHitInfoIteratorAndNary and_iter(std::move(iterators));

  ICING_EXPECT_OK(and_iter.Advance());
  EXPECT_THAT(and_iter.doc_hit_info().hit_section_ids_mask(),
              Eq(mask_ored_result));
  EXPECT_THAT(and_iter.hit_intersect_section_ids_mask(), Eq(mask_anded_result));
}

}  // namespace

}  // namespace lib
}  // namespace icing
